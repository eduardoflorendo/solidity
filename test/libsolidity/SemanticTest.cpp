/*
	This file is part of solidity.
	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <test/libsolidity/SemanticTest.h>
#include <test/Options.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/throw_exception.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <stdexcept>

using namespace dev;
using namespace solidity;
using namespace dev::solidity::test;
using namespace dev::solidity::test::formatting;
using namespace std;
namespace fs = boost::filesystem;
using namespace boost;
using namespace boost::algorithm;
using namespace boost::unit_test;

namespace
{
	string formatBytes(bytes const& _bytes, vector<ABIType> _types, bool const _formatInvalid = false)
	{
		stringstream resultStream;
		auto it = _bytes.begin();
		for (auto const& abiType: _types)
		{
			bytes byteRange{it, it + abiType.size};
			switch (abiType.type)
			{
			case ABIType::SignedDec:
				if (*byteRange.begin() & 0x80)
					resultStream << u2s(fromBigEndian<u256>(byteRange));
				else
					resultStream << fromBigEndian<u256>(byteRange);
				break;
			case ABIType::UnsignedDec:
				// Check if the detected type was wrong and if this could
				// be signed. If an unsigned was detected in the expectations,
				// but the actual result returned a signed, it would be formatted
				// incorrectly.
				if (*byteRange.begin() & 0x80)
					resultStream << u2s(fromBigEndian<u256>(byteRange));
				else
					resultStream << fromBigEndian<u256>(byteRange);
				break;
			case ABIType::Invalid:
				// If expectations are empty, the encoding type is invalid.
				// In order to still print the actual result even if
				// empty expectations were detected, it must be forced.
				if (_formatInvalid)
					resultStream << fromBigEndian<u256>(byteRange);
				break;
			}
			it += abiType.size;
			if (it != _bytes.end())
				resultStream << ", ";
		}
		return resultStream.str();
	}
}

SemanticTest::SemanticTest(string const& _filename, string const& _ipcPath):
	SolidityExecutionFramework(_ipcPath)
{
	ifstream file(_filename);
	if (!file)
		BOOST_THROW_EXCEPTION(runtime_error("Cannot open test contract: \"" + _filename + "\"."));
	file.exceptions(ios::badbit);

	m_source = parseSource(file);
	parseExpectations(file);
}

bool SemanticTest::run(ostream& _stream, string const& _linePrefix, bool const _formatted)
{
	if (!deploy("", 0, bytes()))
		BOOST_THROW_EXCEPTION(runtime_error("Failed to deploy contract."));

	bool success = true;
	for (auto& test: m_tests)
		test.reset();

	for (auto& test: m_tests)
	{
		bytes output = callContractFunctionWithValueNoEncoding(
			test.call.signature,
			test.call.value,
			test.call.arguments.rawBytes
		);

		if ((m_transactionSuccessful != test.call.expectations.status) || (output != test.call.expectations.rawBytes))
			success = false;

		test.status = m_transactionSuccessful;
		test.rawBytes = std::move(output);
	}

	if (!success)
	{
		string nextIndentLevel = _linePrefix + "  " + "// ";
		FormattedScope(_stream, _formatted, {BOLD, CYAN}) << _linePrefix << "Expected result:" << endl;
		for (auto const& test: m_tests)
		{
			printFunctionCallHighlighted(_stream, test.call, nextIndentLevel);
			printFunctionCallTestHighlighted(_stream, test, true, nextIndentLevel, _formatted);
		}

		FormattedScope(_stream, _formatted, {BOLD, CYAN}) << _linePrefix << "Obtained result:" << endl;
		for (auto const& test: m_tests)
		{
			printFunctionCallHighlighted(_stream, test.call, nextIndentLevel);
			printFunctionCallTestHighlighted(_stream, test, false, nextIndentLevel, _formatted);
		}
		return false;
	}
	return true;
}

void SemanticTest::printSource(ostream& _stream, string const& _linePrefix, bool const) const
{
	stringstream stream(m_source);
	string line;
	while (getline(stream, line))
		_stream << _linePrefix << line << endl;
}

void SemanticTest::printUpdatedExpectations(ostream& _stream, string const& _linePrefix) const
{
	for (auto const& test: m_tests)
	{
		printFunctionCallHighlighted(_stream, test.call, _linePrefix);
		printFunctionCallTestHighlighted(_stream, test, false, _linePrefix);
	}
}

void SemanticTest::parseExpectations(istream& _stream)
{
	TestFileParser parser{_stream};
	for (auto const& call: parser.parseFunctionCalls())
		m_tests.emplace_back(FunctionCallTest{call, bytes{}, string{}});
}

bool SemanticTest::deploy(string const& _contractName, u256 const& _value, bytes const& _arguments)
{
	auto output = compileAndRunWithoutCheck(m_source, _value, _contractName, _arguments);
	return !output.empty() && m_transactionSuccessful;
}

void SemanticTest::printFunctionCallHighlighted(ostream& _stream, FunctionCall const& _call, string const& _linePrefix) const
{
	_stream << _linePrefix << _call.signature;
	if (_call.value > u256(0))
		_stream << ", " << _call.value << " " <<TestFileParser::formatToken(SoltToken::Ether);
	if (!_call.arguments.rawBytes.empty())
		_stream << ": " << formatBytes(_call.arguments.rawBytes, _call.arguments.formats);
	if (!_call.arguments.comment.empty())
		_stream << " # " << _call.arguments.comment;
	_stream << endl;
}

void SemanticTest::printFunctionCallTestHighlighted(
	ostream& _stream,
	FunctionCallTest const& _test,
	bool _printExcepted,
	string const& _linePrefix,
	bool const _formatted
) const
{
	auto formatOutput = [](bytes _bytes, vector<ABIType> _types, const bool _isExpectation) -> string
	{
		if (_bytes.empty())
			return TestFileParser::formatToken(SoltToken::Failure);
		else
			return formatBytes(_bytes, _types, !_isExpectation);
	};
	bytes outputBytes = _printExcepted
			? _test.call.expectations.rawBytes
			: _test.rawBytes;
	string output = formatOutput(outputBytes, _test.call.expectations.formats, _printExcepted);

	_stream << _linePrefix;
	if (_formatted && !_test.matchesExpectation())
		_stream << formatting::RED_BACKGROUND;
	_stream << "-> " << output;
	if (_formatted && !_test.matchesExpectation())
		_stream << formatting::RESET;
	if (!_test.call.expectations.comment.empty())
		_stream << " # " << _test.call.expectations.comment;
	_stream << endl;
}

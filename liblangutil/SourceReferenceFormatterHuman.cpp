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
/**
 * Formatting functions for errors referencing positions and locations in the source.
 */

#include <liblangutil/SourceReferenceFormatterHuman.h>
#include <liblangutil/Scanner.h>
#include <liblangutil/Exceptions.h>
#include <cmath>
#include <iomanip>

using namespace std;
using namespace dev;
using namespace dev::formatting;
using namespace langutil;

void SourceReferenceFormatterHuman::printSourceLocation(SourceReference const& _ref)
{
	if (_ref.position.line < 0)
		return; // Nothing we can print here

	int const leftpad = static_cast<int>(log10(max(_ref.position.line, 1))) + 1;

	// line 0: source name
	m_stream
		<< frameColor() << string(leftpad, ' ') << "--> "
		<< normalColor() << _ref.sourceName << ":" << (_ref.position.line + 1) << ":" << (_ref.position.column + 1) << ": "
		<< '\n';

	if (!_ref.multiline)
	{
		int const locationLength = _ref.endColumn - _ref.startColumn;

		// line 1:
		m_stream << string(leftpad, ' ') << frameColor() << " |" << '\n';

		// line 2:
		m_stream
			<< frameColor() << (_ref.position.line + 1) << " | "
			<< normalColor() << _ref.text.substr(0, _ref.startColumn)
			<< highlightColor() << _ref.text.substr(_ref.startColumn, locationLength)
			<< normalColor() << _ref.text.substr(_ref.endColumn) << '\n';

		// line 3:
		m_stream << string(leftpad, ' ') << frameColor() << " | ";
		for_each(
			_ref.text.cbegin(),
			_ref.text.cbegin() + _ref.startColumn,
			[this](char ch) { m_stream << (ch == '\t' ? '\t' : ' '); }
		);
		m_stream << diagColor() << string(locationLength, '^') << '\n';
	}
	else
	{
		// line 1:
		m_stream << string(leftpad, ' ') << frameColor() << " |" << '\n';

		// line 2:
		m_stream
			<< frameColor() << (_ref.position.line + 1) << " | "
			<< normalColor() << _ref.text.substr(0, _ref.startColumn)
			<< highlightColor() << _ref.text.substr(_ref.startColumn) << '\n';

		// line 3:
		m_stream
			<< frameColor() << string(leftpad, ' ') << " | "
			<< normalColor() << string(_ref.startColumn, ' ')
			<< diagColor() << "^ (Relevant source part starts here and spans across multiple lines).";
	}

	m_stream << '\n';
}

void SourceReferenceFormatterHuman::printExceptionInformation(SourceReferenceExtractor::Message const& _msg)
{
	// exception header line
	m_stream
		<< errorColor() << _msg.category
		<< messageColor() << ": " << _msg.primary.message << '\n';

	if (_msg.primary.position.line < 0 || _msg.primary.sourceName.empty())
		m_stream << '\n';

	printSourceLocation(_msg.primary);

	for (auto const& secondary : _msg.secondary)
	{
		m_stream << secondaryColor() << "Note" << messageColor() << ": " << secondary.message << '\n';
		printSourceLocation(secondary);
	}
}

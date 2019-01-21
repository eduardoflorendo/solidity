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

#include <libyul/optimiser/VarNameCleaner.h>
#include <libyul/AsmData.h>
#include <libyul/Dialect.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <iterator>
#include <string>
#include <regex>

using namespace yul;
using namespace std;

// ensure the following transforms
// - input: a, a_1, a_1_2
//	 output: a, a_1, a_2
// - input: a, a_1, a_1_2, a_2
//   output: a, a_1, a_2, a_3
// - input: a_15, a_17
//   output: a, a_2
// - input: abi_decode_256
//   output: abi_decode

void VarNameCleaner::operator()(VariableDeclaration& _varDecl)
{
	for (TypedName& typedName: _varDecl.variables)
		if (auto newName = makeCleanName(typedName.name.str()))
			typedName.name = YulString{*newName};

	ASTModifier::operator()(_varDecl);
}

void VarNameCleaner::operator()(Identifier& _identifier)
{
	if (auto newName = getCleanName(_identifier.name.str()))
		_identifier.name = YulString{*newName};
}

boost::optional<string> VarNameCleaner::makeCleanName(string const& _name)
{
	if (auto newName = findCleanName(_name))
	{
		m_usedNames[*newName] = *newName;
		m_usedNames[_name] = *newName;

		return newName;
	}

	// Name isn't used yet, but we need to make sure nobody else does.
	m_usedNames[_name] = _name;
	return boost::none;
}

// make identifier clean, remember new name for reuse, keep them unique and simple
boost::optional<string> VarNameCleaner::findCleanName(string const& _name) const
{
	if (auto newName = stripSuffix(_name))
	{
		if (newName->length() != 0 && !m_dialect.builtin(YulString{*newName}) && !m_usedNames.count(*newName))
			return newName;

		// create new name with suffix (by finding a free identifier)
		for (size_t i = 1; i < numeric_limits<decltype(i)>::max(); ++i)
		{
			string newNameSuffixed = *newName + "_" + to_string(i);
			if (!m_usedNames.count(newNameSuffixed))
				if (!binary_search(begin(m_blacklist), end(m_blacklist), newNameSuffixed))
					return newNameSuffixed;
		}
		yulAssert(false, "Exhausted by attempting to find an available suffix.");
	}

	return boost::none;
}

boost::optional<std::string> VarNameCleaner::stripSuffix(string const& _name) const
{
	static std::regex const suffixRegex("(_+[0-9]+)+$");

	std::smatch suffixMatch;
	if (std::regex_search(_name, suffixMatch, suffixRegex))
	{
		std::string name = suffixMatch.prefix().str();
		if (!binary_search(begin(m_blacklist), end(m_blacklist), name))
			return {name};
	}

	return boost::none;
}

boost::optional<string> VarNameCleaner::getCleanName(string const& _name) const
{
	auto n = m_usedNames.find(_name);
	if (n != m_usedNames.end() && n->second != _name)
		// Mapping found *and* name was changed.
		return {n->second};
	else
		// No mapping found or name didn't change.
		return boost::none;
}

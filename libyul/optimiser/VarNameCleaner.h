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

#pragma once


#include <libyul/AsmDataForward.h>
#include <libyul/optimiser/ASTWalker.h>

#include <boost/optional.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace yul
{

struct Dialect;

/**
 * Pass to clean variable names with hard-to-read names, that happened due to
 * disambiguation, such as stripping and normalizing suffixes, if possible.
 *
 * Prerequisites: Disambiguator
 */
class VarNameCleaner: public ASTModifier
{
public:
	using BlackList = std::vector<std::string>;

	VarNameCleaner(Dialect const& _dialect, BlackList _blacklist = {})
		: m_dialect{_dialect}, m_blacklist{std::move(_blacklist)}
	{
		// make sure it's sorted, so we can use binary search later.
		std::sort(begin(m_blacklist), end(m_blacklist));
	}

	using ASTModifier::operator();
	void operator()(VariableDeclaration& _varDecl) override;
	void operator()(Identifier&) override;

private:
	/// @returns suffix-stripped name, if a suffix was detected, none otherwise.
	boost::optional<std::string> stripSuffix(std::string const& _name) const;

	/// Looks out for a "clean name" the given @p name could be trimmed down to.
	/// @returns a trimmed down and "clean name" in case it found one, none otherwise.
	boost::optional<std::string> findCleanName(std::string const& name) const;

	/// Uses findCleanName to find a clean name, and then remembers it, so future calls
	/// don't pick that name for their use.
	///
	/// @returns a trimmed down and "clean name" in case it found one, none otherwise.
	boost::optional<std::string> makeCleanName(std::string const& name);

	/// Retruens the new name, if one was mapped, or none.
	boost::optional<std::string> getCleanName(std::string const& _name) const;

	/// map on old name to new name
	std::unordered_map<std::string, std::string> m_usedNames;

	Dialect const& m_dialect;
	BlackList m_blacklist;
};

}

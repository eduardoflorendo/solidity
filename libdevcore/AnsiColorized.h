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

#include <ostream>
#include <vector>

namespace dev
{

namespace formatting
{

// control codes
static constexpr char const* RESET = "\033[0m";
static constexpr char const* INVERSE = "\033[7m";
static constexpr char const* BOLD = "\033[1m";
static constexpr char const* BRIGHT = BOLD;

// standard foreground colors
static constexpr char const* BLACK = "\033[30m";
static constexpr char const* RED = "\033[31m";
static constexpr char const* GREEN = "\033[32m";
static constexpr char const* YELLOW = "\033[33m";
static constexpr char const* BLUE = "\033[34m";
static constexpr char const* MAGENTA = "\033[35m";
static constexpr char const* CYAN = "\033[36m";
static constexpr char const* WHITE = "\033[37m";

// standard background colors
static constexpr char const* BLACK_BACKGROUND = "\033[40m";
static constexpr char const* RED_BACKGROUND = "\033[41m";
static constexpr char const* GREEN_BACKGROUND = "\033[42m";
static constexpr char const* YELLOW_BACKGROUND = "\033[43m";
static constexpr char const* BLUE_BACKGROUND = "\033[44m";
static constexpr char const* MAGENTA_BACKGROUND = "\033[45m";
static constexpr char const* CYAN_BACKGROUND = "\033[46m";
static constexpr char const* WHITE_BACKGROUND = "\033[47m";

// 256-bit-colors (incomplete set)
static constexpr char const* RED_BACKGROUND_256 = "\033[48;5;160m";
static constexpr char const* ORANGE_BACKGROUND_256 = "\033[48;5;166m";

}

/// AnsiColorized provides a convenience helper to colorize ostream with formatting-reset assured.
class AnsiColorized
{
public:
	AnsiColorized& operator=(AnsiColorized const&) = delete;
	AnsiColorized(AnsiColorized&) = delete;

	AnsiColorized& operator=(AnsiColorized&& other)
	{
		m_stream = other.m_stream;
		m_enabled = other.m_enabled;
		m_codes = std::move(other.m_codes);
		other.m_enabled = false;
		return *this;
	}

	AnsiColorized(AnsiColorized&& other):
		m_stream{other.m_stream}, m_enabled{other.m_enabled}, m_codes{std::move(other.m_codes)}
	{
		other.m_enabled = false;
	}

	/// @arg _formatting List of formatting strings (e.g. colors) defined in the formatting namespace.
	AnsiColorized(bool const _enabled, std::vector<char const*>&& _formatting):
		m_stream{nullptr}, m_enabled{_enabled}, m_codes{std::move(_formatting)}
	{
	}

	~AnsiColorized()
	{
		if (m_enabled && m_stream)
			*m_stream << formatting::RESET;
	}

	/// Applies format on given stream and assures formatting-reset at object destruction.
	/// @note Please ensure you only invoke it once, as it can only reset colors on one (last) stream.
	void apply(std::ostream& os) const
	{
		if (m_enabled)
		{
			m_stream = &os;
			for (auto const& code: m_codes)
				os << code;
		}
	}

private:
	mutable std::ostream* m_stream;
	bool m_enabled;
	std::vector<char const*> m_codes;
};

inline std::ostream& operator<<(std::ostream& os, AnsiColorized const& formats)
{
	formats.apply(os);
	return os;
}

}

//  retcon
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2015 - Jonathan G Rennison <j.g.rennison@gmail.com>
//==========================================================================

#ifndef HGUARD_SRC_EMOJI_TWEMOJI
#define HGUARD_SRC_EMOJI_TWEMOJI

#include "../univdefs.h"
#include <string>
#include <utility>

extern const std::string emoji_regex;

struct emoji_item {
	uint32_t first;
	uint32_t second;
	std::pair<const unsigned char *, const unsigned char *> ptrs_16;
	std::pair<const unsigned char *, const unsigned char *> ptrs_36;
};

extern const emoji_item emoji_map[];
extern const size_t emoji_map_size;

#endif

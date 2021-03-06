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
//  2012 - Jonathan G Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "univdefs.h"
#include "version.h"

#ifndef RETCON_BUILD_VERSION
#define RETCON_BUILD_VERSION RETCON_VERSION_STR __DATE__ " " __TIME__
#endif

const wxString appname(wxT("retcon"));
const wxString appversionname(wxT("Retcon v") RETCON_VERSION_STR);
const wxString appbuildversion(wxT(RETCON_BUILD_VERSION));

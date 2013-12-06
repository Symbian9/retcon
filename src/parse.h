//  retcon
//
//  WEBSITE: http://retcon.sourceforge.net
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

#ifndef HGUARD_SRC_PARSE
#define HGUARD_SRC_PARSE

#include "univdefs.h"
#include "rapidjson-inc.h"
#include "twit-common.h"
#include <wx/version.h>
#include <wx/defs.h>
#include <wx/string.h>
#include <string>
#include <memory>

struct dbsendmsg_list;
struct taccount;
struct twitcurlext;
struct tweet;
struct userdata;

#if wxCHECK_GCC_VERSION(4, 6)	//in old gccs, just leave the warnings turned off
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "rapidjson/writer.h"
#include "rapidjson/reader.h"
#if wxCHECK_GCC_VERSION(4, 6)
#pragma GCC diagnostic pop
#endif

struct writestream {
	writestream(std::string &str_, size_t reshint=512 ) : str(str_) { str.clear(); str.reserve(reshint); }
	std::string &str;
	inline void Put(char ch) { str.push_back(ch); }
};

struct Handler : public rapidjson::Writer<writestream> {
	using rapidjson::Writer<writestream>::String;
	Handler& String(const std::string &str) {
		rapidjson::Writer<writestream>::String(str.c_str(), str.size());
		return *this;
	}
	Handler(writestream &wr) : rapidjson::Writer<writestream>::Writer(wr) { }
};

struct genjsonparser {
	static void ParseTweetStatics(const rapidjson::Value& val, const std::shared_ptr<tweet> &tobj, Handler *jw=0, bool isnew=false, dbsendmsg_list *dbmsglist=0, bool parse_entities=true);
	static void DoEntitiesParse(const rapidjson::Value& val, const std::shared_ptr<tweet> &t, bool isnew=false, dbsendmsg_list *dbmsglist=0);
	static void ParseUserContents(const rapidjson::Value& val, userdata &userobj, bool is_ssl=0);
	static void ParseTweetDyn(const rapidjson::Value& val, const std::shared_ptr<tweet> &tobj);
};

enum {
	JDTP_ISDM	= 1<<0,
	JDTP_ISRTSRC	= 1<<1,
	JDTP_FAV	= 1<<2,
	JDTP_UNFAV	= 1<<3,
	JDTP_DEL	= 1<<4,
	JDTP_USERTIMELINE	= 1<<5,
	JDTP_CHECKPENDINGONLY	= 1<<6
};

struct jsonparser : public genjsonparser {
	std::shared_ptr<taccount> tac;
	CS_ENUMTYPE type;
	twitcurlext *twit;
	rapidjson::Document dc;
	dbsendmsg_list *dbmsglist;

	std::shared_ptr<userdatacontainer> DoUserParse(const rapidjson::Value& val, unsigned int umpt_flags=0);
	void DoEventParse(const rapidjson::Value& val);
	void DoFriendLookupParse(const rapidjson::Value& val);
	std::shared_ptr<tweet> DoTweetParse(const rapidjson::Value& val, unsigned int sflags=0);
	void RestTweetUpdateParams(const tweet &t);
	void RestTweetPreParseUpdateParams();

	jsonparser(CS_ENUMTYPE t, std::shared_ptr<taccount> a, twitcurlext *tw = 0 /*optional*/)
		: tac(a), type(t), twit(tw), dbmsglist(0) { }
	bool ParseString(const char *str, size_t len);
	bool ParseString(const std::string &str) { return ParseString(str.c_str(), str.size()); }
};

void DisplayParseErrorMsg(rapidjson::Document &dc, const wxString &name, const char *data);

#endif

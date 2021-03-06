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
#include "userui.h"
#include "mainui.h"
#include "taccount.h"
#include "twitcurlext.h"
#include "twit.h"
#include "tpanel.h"
#include "tpanel-data.h"
#include "userui.h"
#include "util.h"
#include "alldata.h"
#include "log.h"
#include "log-util.h"
#include "bind_wxevt.h"
#include "db.h"
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/grid.h>

std::unordered_map<uint64_t, user_window*> userwinmap;

struct user_window::event_log_entry {
	std::weak_ptr<taccount> acc;
	DB_EVENTLOG_TYPE type;
	time_t eventtime;
	uint64_t obj_id;
};

BEGIN_EVENT_TABLE(notebook_event_prehandler, wxEvtHandler)
	EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, notebook_event_prehandler::OnPageChange)
END_EVENT_TABLE()

void notebook_event_prehandler::OnPageChange(wxNotebookEvent &event) {
	int i = event.GetSelection();
	if (i >= 0) {
		for (auto &it : timeline_pane_list) {
			if (nb->GetPage(i) == it) {
				it->InitLoading();
			}
		}
		for (auto &it : userlist_pane_list) {
			if (nb->GetPage(i) == it) {
				it->InitLoading();
			}
		}
	}
	event.Skip();
	uw->Layout();
	uw->Fit();
}

BEGIN_EVENT_TABLE(user_window, wxDialog)
	EVT_CLOSE(user_window::OnClose)
	EVT_CHOICE(wxID_FILE1, user_window::OnSelChange)
	EVT_BUTTON(FOLLOWBTN_ID, user_window::OnFollowBtn)
	EVT_MENU(REFRESHBTN_ID, user_window::OnRefreshBtn)
	EVT_BUTTON(MOREBTN_ID, user_window::OnMoreBtn)
	EVT_BUTTON(DMBTN_ID, user_window::OnDMBtn)
	EVT_MENU(DMCONVERSATIONBTN_ID, user_window::OnDMConversationBtn)
	EVT_TEXT(NOTESTXT_ID, user_window::OnNotesTextChange)
END_EVENT_TABLE()

static void insert_uw_row(wxWindow *parent, wxSizer *sz, const wxString &label, wxStaticText *&targ) {
	wxStaticText *name = new wxStaticText(parent, wxID_ANY, label);
	wxStaticText *data = new wxStaticText(parent, wxID_ANY, wxT(""));
	sz->Add(name, 0, wxALL, 2);
	sz->Add(data, 0, wxALL, 2);
	targ = data;
}

static void insert_block_state_text(wxWindow *parent, wxSizer *sz, const wxString &label, wxStaticText *&targ) {
	wxStaticText *name = new wxStaticText(parent, wxID_ANY, label);
	wxFont font = name->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	name->SetFont(font);
	sz->Add(name, 0, wxALL, 4);
	targ = name;
}

user_window::user_window(uint64_t userid_, const std::shared_ptr<taccount> &acc_hint_)
		: wxDialog(0, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT), userid(userid_), acc_hint(acc_hint_) {
	evtbinder.reset(new bindwxevt_win(this));
	userwinmap[userid_] = this;
	u = ad.GetUserContainerById(userid_);
	u->udc_flags |= UDC::WINDOWOPEN;
	u->ImgIsReady(PENDING_REQ::PROFIMG_DOWNLOAD);
	CheckAccHint();

	// Only need to do this once, user_window will hold a ref to it so it won't be evicted thereafter
	CheckIfUserAlreadyInDBAndLoad(u);

	std::shared_ptr<taccount> acc = acc_hint.lock();
	if (acc && acc->enabled && u->NeedsUpdating(0) && !(u->udc_flags & UDC::LOOKUP_IN_PROGRESS) && !(u->udc_flags & UDC::BEING_LOADED_FROM_DB)) {
		acc->MarkUserPending(u);
		acc->StartRestQueryPendings();
	}

	Freeze();

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *headerbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(headerbox, 0, wxALL | wxEXPAND, 4);
	usericon = new wxStaticBitmap(this, wxID_ANY, u->cached_profile_img, wxPoint(-1000, -1000));
	headerbox->Add(usericon, 0, wxALL, 2);
	wxBoxSizer *infobox = new wxBoxSizer(wxVERTICAL);
	headerbox->Add(infobox, 0, wxALL, 2);
	name = new wxStaticText(this, wxID_ANY, wxT(""));
	screen_name = new wxStaticText(this, wxID_ANY, wxT(""));
	infobox->Add(name, 0, wxALL, 2);
	infobox->Add(screen_name, 0, wxALL, 2);

	wxStaticBoxSizer *sb = new wxStaticBoxSizer(wxHORIZONTAL, this, wxT("Account"));
	wxBoxSizer *sbvbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(sb, 0, wxALL | wxEXPAND, 2);
	sb->Add(sbvbox, 0, 0, 0);
	accchoice = new wxChoice(this, wxID_FILE1);
	sbvbox->Add(accchoice, 0, wxALL, 2);
	fill_accchoice();
	follow_grid = new wxFlexGridSizer(0, 2, 2, 2);
	sbvbox->Add(follow_grid, 0, wxALL, 2);
	insert_uw_row(this, follow_grid, wxT("Following:"), ifollow);
	insert_uw_row(this, follow_grid, wxT("Followed By:"), followsme);
	insert_block_state_text(this, sbvbox, wxT("This user is blocked"), is_blocked);
	insert_block_state_text(this, sbvbox, wxT("This user is muted"), is_muted);
	insert_block_state_text(this, sbvbox, wxT("Retweets are disabled for this user"), is_no_rt);
	sb->AddStretchSpacer();
	wxBoxSizer *accbuttonbox = new wxBoxSizer(wxVERTICAL);
	sb->Add(accbuttonbox, 0, wxALIGN_RIGHT | wxALIGN_TOP, 0);
	followbtn = new wxButton(this, FOLLOWBTN_ID, wxT(""));
	dmbtn = new wxButton(this, DMBTN_ID, wxT("Send DM"));
	morebtn = new wxButton(this, MOREBTN_ID, wxT("More \x25BC"));
	accbuttonbox->Add(followbtn, 0, wxEXPAND | wxALIGN_TOP, 0);
	accbuttonbox->Add(dmbtn, 0, wxEXPAND | wxALIGN_TOP, 0);
	accbuttonbox->Add(morebtn, 0, wxEXPAND | wxALIGN_TOP, 0);
	follow_btn_mode = FOLLOWBTNMODE::FBM_NONE;

	nb = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxNB_TOP | wxNB_NOPAGETHEME | wxNB_MULTILINE);
	nb_prehndlr.uw = this;

	wxPanel *infopanel = new wxPanel(nb, wxID_ANY);
	vbox->Add(nb, 0, wxALL | wxEXPAND, 4);
	if_grid = new wxFlexGridSizer(0, 2, 2, 2);
	infopanel->SetSizer(if_grid);
	insert_uw_row(infopanel, if_grid, wxT("Name:"), name2);
	insert_uw_row(infopanel, if_grid, wxT("Screen Name:"), screen_name2);
	insert_uw_row(infopanel, if_grid, wxT("Description:"), desc);
	insert_uw_row(infopanel, if_grid, wxT("Location:"), location);
	insert_uw_row(infopanel, if_grid, wxT("Protected:"), isprotected);
	insert_uw_row(infopanel, if_grid, wxT("Verified Account:"), isverified);
	insert_uw_row(infopanel, if_grid, wxT("Tweets:"), tweets);
	insert_uw_row(infopanel, if_grid, wxT("Followers:"), followers);
	insert_uw_row(infopanel, if_grid, wxT("Following:"), follows);
	insert_uw_row(infopanel, if_grid, wxT("Has Favourited:"), faved);

	url_label = new wxStaticText(infopanel, wxID_ANY, wxT("Web URL:"));
	if_grid->Add(url_label, 0, wxALL, 2);
	url = new wxHyperlinkCtrl(infopanel, wxID_ANY, wxT("url"), wxT("url"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHL_CONTEXTMENU|wxHL_ALIGN_LEFT);
	if_grid->Add(url, 0, wxALL, 2);

	if_grid->Add(new wxStaticText(infopanel, wxID_ANY, wxT("Profile URL:")), 0, wxALL, 2);
	profileurl = new wxHyperlinkCtrl(infopanel, wxID_ANY, wxT("url"), wxT("url"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHL_CONTEXTMENU|wxHL_ALIGN_LEFT);
	if_grid->Add(profileurl, 0, wxALL, 2);

	insert_uw_row(infopanel, if_grid, wxT("Creation Time:"), createtime);
	insert_uw_row(infopanel, if_grid, wxT("Last Updated:"), lastupdate);
	insert_uw_row(infopanel, if_grid, wxT("Account ID:"), id_str);

	nb->AddPage(infopanel, wxT("Info"), true);

	safe_observer_ptr<user_window> safe_win_ptr(this);
	std::function<std::shared_ptr<taccount>()> getacc = [safe_win_ptr]() -> std::shared_ptr<taccount> {
		std::shared_ptr<taccount> uw_acc;
		user_window *uw = safe_win_ptr.get();
		if (uw) {
			uw_acc = uw->acc_hint.lock();
		}
		return uw_acc;
	};
	auto getacc_tw = [getacc](tpanelparentwin_usertweets &src) -> std::shared_ptr<taccount> { return getacc(); };
	auto getacc_prop = [getacc](tpanelparentwin_userproplisting &src) -> std::shared_ptr<taccount> { return getacc(); };

	timeline_pane = new tpanelparentwin_usertweets(u, nb, getacc_tw);
	nb->AddPage(timeline_pane, wxT("Timeline"), false);
	fav_timeline_pane = new tpanelparentwin_usertweets(u, nb, getacc_tw, RBFS_USER_FAVS);
	nb->AddPage(fav_timeline_pane, wxT("Favourites"), false);
	followers_pane = new tpanelparentwin_userproplisting(u, nb, getacc_prop, tpanelparentwin_userproplisting::TYPE::USERFOLLOWERS);
	nb->AddPage(followers_pane, wxT("Followers"), false);
	friends_pane = new tpanelparentwin_userproplisting(u, nb, getacc_prop, tpanelparentwin_userproplisting::TYPE::USERFOLLOWING);
	nb->AddPage(friends_pane, wxT("Following"), false);

	nb_prehndlr.timeline_pane_list.push_back(timeline_pane);
	nb_prehndlr.timeline_pane_list.push_back(fav_timeline_pane);
	nb_prehndlr.userlist_pane_list.push_back(followers_pane);
	nb_prehndlr.userlist_pane_list.push_back(friends_pane);
	nb_prehndlr.nb = nb;
	nb->PushEventHandler(&nb_prehndlr);

	nb_tab_insertion_point = nb->GetPageCount();

	notes_tab = new wxPanel(nb, wxID_ANY);
	wxSizer *notes_size = new wxBoxSizer(wxVERTICAL);
	notes_tab->SetSizer(notes_size);
	wxString notes_header = wxT("Private Notes:\nThese notes will not be uploaded to Twitter\nand will not be visible to this or other users.");
	notes_size->Add(new wxStaticText(notes_tab, wxID_ANY, notes_header), 0, wxALL, 2);
	notes_txt = new wxTextCtrl(notes_tab, NOTESTXT_ID, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	notes_size->Add(notes_txt, 1, wxALL | wxEXPAND, 2);
	nb->AddPage(notes_tab, wxT("Notes"));

	SetSizer(vbox);

	Refresh(false, true);
	Thaw();

	if (uwt_common.expired()) {
		uwt = std::make_shared<user_window_timer>();
		uwt_common = uwt;
		uwt->Start(90000, false);
	} else {
		uwt = uwt_common.lock();
	}

	Show();
}

void user_window::OnSelChange(wxCommandEvent &event) {
	int selection = accchoice->GetSelection();
	acc_hint.reset();
	if (selection != wxNOT_FOUND) {
		taccount *acc = static_cast<taccount *>(accchoice->GetClientData(selection));
		for (auto it = alist.begin(); it != alist.end(); it++) {
			if ((*it).get() == acc) {
				acc_hint = *it;
				break;
			}
		}
		RefreshAccState();
		wxNotebookEvent evt(wxEVT_NULL, nb->GetId(), nb->GetSelection(), nb->GetSelection());
		nb_prehndlr.OnPageChange(evt);
	} else {
		RefreshAccState();
	}
}

void user_window::RefreshAccState() {
	std::shared_ptr<taccount> acc = acc_hint.lock();
	bool isownacc = (acc && acc->usercont == u);
	follow_grid->Show(!isownacc);
	followbtn->Show(!isownacc);
	dmbtn->Show(!isownacc);
	if (!isownacc) {
		RefreshFollow();
	} else {
		is_blocked->Show(false);
		is_muted->Show(false);
		is_no_rt->Show(false);
	}

	bool should_have_incoming_pane = isownacc && u->GetUser().u_flags & userdata::UF::ISPROTECTED;
	bool should_have_outgoing_pane = isownacc;

	if (should_have_incoming_pane && !incoming_pane) {
		incoming_pane = new tpanelparentwin_userproplisting(u, nb, [acc](tpanelparentwin_userproplisting &) { return acc; }, tpanelparentwin_userproplisting::TYPE::OWNINCOMINGFOLLOWLISTING);
		nb->InsertPage(nb_tab_insertion_point, incoming_pane, wxT("Incoming"), false);
		nb_prehndlr.userlist_pane_list.push_back(incoming_pane);
	}
	if (should_have_outgoing_pane && !outgoing_pane) {
		outgoing_pane = new tpanelparentwin_userproplisting(u, nb, [acc](tpanelparentwin_userproplisting &) { return acc; }, tpanelparentwin_userproplisting::TYPE::OWNOUTGOINGFOLLOWLISTING);
		nb->InsertPage(nb_tab_insertion_point, outgoing_pane, wxT("Outgoing"), false);
		nb_prehndlr.userlist_pane_list.push_back(outgoing_pane);
	}

	auto remove_page = [&](tpanelparentwin_userproplisting *&ptr) {
		container_unordered_remove(nb_prehndlr.userlist_pane_list, ptr);
		for (size_t i = 0; i < nb->GetPageCount(); i++) {
			if (nb->GetPage(i) == ptr) {
				nb->DeletePage(i);
				ptr = nullptr;
				break;
			}
		}
	};
	if (!should_have_incoming_pane && incoming_pane) {
		remove_page(incoming_pane);
	}
	if (!should_have_outgoing_pane && outgoing_pane) {
		remove_page(outgoing_pane);
	}
	SetNotebookMinSize();
	Layout();
	Fit();
}

void user_window::fill_accchoice() {
	std::shared_ptr<taccount> acc = acc_hint.lock();
	for (auto &it : alist) {
		wxString accname = it->dispname;
		if (!it->enabled) {
			accname += wxT(" [disabled]");
		}
		accchoice->Append(accname, it.get());
		if (it.get() == acc.get()) {
			accchoice->SetSelection(accchoice->GetCount() - 1);
		}
	}
}

static void set_uw_time_val(wxStaticText *st, const time_t &input) {
	if (input) {
		time_t updatetime;	//not used
		wxString val = wxString::Format(wxT("%s (%s)"), getreltimestr(input, updatetime).c_str(), cfg_strftime(input).c_str());
		st->SetLabel(val);
	} else {
		st->SetLabel(wxT(""));
	}
}

void user_window::RefreshFollow(bool forcerefresh) {
	using URF = user_relationship::URF;
	std::shared_ptr<taccount> acc = acc_hint.lock();
	bool needupdate = forcerefresh;
	FOLLOWBTNMODE fbm = FOLLOWBTNMODE::FBM_NONE;

	auto fill_follow_field = [&](wxStaticText *st, bool ifollow) {
		wxString value;
		if (acc) {
			bool known = false;
			auto it = acc->user_relations.find(userid);
			if (it != acc->user_relations.end()) {
				user_relationship &ur = it->second;
				if (ur.ur_flags & (ifollow ? URF::IFOLLOW_KNOWN : URF::FOLLOWSME_KNOWN)) {
					known = true;
					if (ur.ur_flags&(ifollow ? URF::IFOLLOW_TRUE : URF::FOLLOWSME_TRUE)) {
						if (ifollow) fbm = FOLLOWBTNMODE::FBM_UNFOLLOW;
						value = wxT("Yes");
					} else if (ur.ur_flags&(ifollow ? URF::IFOLLOW_PENDING : URF::FOLLOWSME_PENDING)) {
						if (ifollow) fbm = FOLLOWBTNMODE::FBM_REMOVE_PENDING;
						value = wxT("Pending");
					} else {
						if (ifollow) fbm = FOLLOWBTNMODE::FBM_FOLLOW;
						value = wxT("No");
					}

					time_t updtime = ifollow ? ur.ifollow_updtime : ur.followsme_updtime;
					if (updtime && (time(nullptr) - updtime) > 180) {
						time_t updatetime;	//not used
						value = wxString::Format(wxT("%s as of %s (%s)"), value.c_str(), getreltimestr(updtime, updatetime).c_str(), cfg_strftime(updtime).c_str());
					}
					st->SetLabel(value);
				}
			}
			if (!known) {
				if (acc->ta_flags & taccount::TAF::STREAM_UP && ifollow) {
					st->SetLabel(wxT("No or Pending"));
				} else {
					st->SetLabel(wxT("Unknown"));
				}
				fbm = FOLLOWBTNMODE::FBM_NONE;
				needupdate = true;
			}
		} else {
			st->SetLabel(wxT("No Account"));
			needupdate = true;
		}

	};

	fill_follow_field(ifollow, true);
	fill_follow_field(followsme, false);

	is_blocked->Show(acc->blocked_users.count(userid));
	is_muted->Show(acc->muted_users.count(userid));
	is_no_rt->Show(acc->no_rt_users.count(userid));

	switch (fbm) {
		case FOLLOWBTNMODE::FBM_UNFOLLOW:
			followbtn->SetLabel(wxT("Unfollow"));
			break;

		case FOLLOWBTNMODE::FBM_REMOVE_PENDING:
			//followbtn->SetLabel(wxT("Cancel Follow Request"));
			//break;	//not implemented in twitter API
		case FOLLOWBTNMODE::FBM_FOLLOW:
		case FOLLOWBTNMODE::FBM_NONE:
			followbtn->SetLabel(wxT("Follow"));
			break;
	}

	followbtn->Enable(acc && acc->enabled && fbm != FOLLOWBTNMODE::FBM_NONE && fbm != FOLLOWBTNMODE::FBM_REMOVE_PENDING && !(u->udc_flags & UDC::FRIENDACT_IN_PROGRESS));
	dmbtn->Enable(acc && acc->enabled);
	follow_btn_mode = fbm;

	if (needupdate && acc && acc->enabled) {
		acc->LookupFriendships(userid);
	}

	Fit();
}

void user_window::Refresh(bool refreshimg, bool refresh_events) {
	LogMsgFormat(LOGT::OTHERTRACE, "user_window::Refresh %" llFmtSpec "d, refreshimg: %d", u->id, refreshimg);
	u->ImgIsReady(PENDING_REQ::PROFIMG_DOWNLOAD); //This will trigger asynchronous (down)load of the image if it is not already ready

	SetTitle(wxT("@") + wxstrstd(u->GetUser().screen_name) + wxT(" (") + wxstrstd(u->GetUser().name) + wxT(")"));
	name->SetLabel(wxstrstd(u->GetUser().name));
	screen_name->SetLabel(wxT("@") + wxstrstd(u->GetUser().screen_name));
	name2->SetLabel(wxstrstd(u->GetUser().name));
	screen_name2->SetLabel(wxT("@") + wxstrstd(u->GetUser().screen_name));
	desc->SetLabel(wxstrstd(u->GetUser().description).Trim());
	desc->Wrap(150);
	location->SetLabel(wxstrstd(u->GetUser().location).Trim());
	isprotected->SetLabel((u->GetUser().u_flags & userdata::userdata::UF::ISPROTECTED)?wxT("Yes"):wxT("No"));
	isverified->SetLabel((u->GetUser().u_flags & userdata::userdata::UF::ISVERIFIED)?wxT("Yes"):wxT("No"));
	tweets->SetLabel(wxString::Format(wxT("%d"), u->GetUser().statuses_count));
	followers->SetLabel(wxString::Format(wxT("%d"), u->GetUser().followers_count));
	follows->SetLabel(wxString::Format(wxT("%d"), u->GetUser().friends_count));
	faved->SetLabel(wxString::Format(wxT("%d"), u->GetUser().favourites_count));
	notes_txt->ChangeValue(wxstrstd(u->GetUser().notes));
	SetNotesTabTitle();

	bool showurl = !u->GetUser().userurl.empty();
	if (showurl) {
		wxString wurl = wxstrstd(u->GetUser().userurl);
		url->SetLabel(wurl);
		url->SetURL(wurl);

	} else {
		url->SetLabel(wxT("<empty>"));
		url->SetURL(wxT("<empty>"));
	}
	if_grid->Show(url, showurl);
	if_grid->Show(url_label, showurl);

	wxString profurl = wxstrstd(u->GetPermalink(true));
	profileurl->SetLabel(profurl);
	profileurl->SetURL(profurl);
	set_uw_time_val(createtime, u->GetUser().createtime);
	set_uw_time_val(lastupdate, (time_t) u->lastupdate);
	id_str->SetLabel(wxString::Format(wxT("%" wxLongLongFmtSpec "d"), u->id));
	if (refreshimg) {
		usericon->SetBitmap(u->cached_profile_img);
	}
	if (refresh_events) {
		uint64_t obj_id = userid;
		int acc_db_index = -1;
		std::shared_ptr<taccount> user_acc = u->GetAccountOfUser();
		if (user_acc) {
			acc_db_index = user_acc->dbindex;
		}
		DBC_AsyncSelEventLogByObj(userid, acc_db_index, [obj_id, acc_db_index](std::deque<dbeventlogdata> data) {
			user_window *win = user_window::GetWin(obj_id);
			if (!win) return;

			win->events.clear();
			auto ready = std::make_shared<exec_on_ready>();
			for (auto &it : data) {
				std::shared_ptr<taccount> acc;
				GetAccByDBIndex(it.accid, acc);
				win->events.push_back({ acc, it.event_type, it.eventtime, it.obj });
				if (it.obj != obj_id) {
					ready->UserReady(ad.GetUserContainerById(it.obj), exec_on_ready::EOR_UR::CHECK_DB, acc);
				}
			}
			ready->Execute([obj_id, acc_db_index]() {
				user_window *win = user_window::GetWin(obj_id);
				if (win) win->EventListUpdated(acc_db_index >= 0);
			});
		});
	}

	RefreshAccState();
	Layout();
}


user_window::~user_window() {
	nb->PopEventHandler();
	userwinmap.erase(userid);
	u->udc_flags &= ~UDC::WINDOWOPEN;
}

void user_window::CheckAccHint() {
	std::shared_ptr<taccount> acc_sp = acc_hint.lock();
	u->GetUsableAccount(acc_sp);
	acc_hint = acc_sp;
}

void user_window::OnClose(wxCloseEvent &event) {
	Destroy();
}

void user_window::OnRefreshBtn(wxCommandEvent &event) {
	std::shared_ptr<taccount> acc = acc_hint.lock();
	if (acc && acc->enabled && !(u->udc_flags & UDC::LOOKUP_IN_PROGRESS) && !(u->udc_flags & UDC::BEING_LOADED_FROM_DB)) {
		acc->MarkUserPending(u);
		u->udc_flags |= UDC::FORCE_REFRESH;
		acc->StartRestQueryPendings();
		RefreshFollow(true);
	}
}

void user_window::OnMoreBtn(wxCommandEvent &event) {
	std::shared_ptr<taccount> acc = acc_hint.lock();
	wxRect btnrect = morebtn->GetRect();
	wxMenu pmenu;

	wxMenuItem *refresh_item = pmenu.Append(REFRESHBTN_ID, wxT("&Refresh"));
	refresh_item->Enable(acc && acc->enabled);

	optional_observer_ptr<user_dm_index> dm_index = ad.GetExistingUserDMIndexById(userid);
	if (dm_index && !dm_index->ids.empty()) {
		pmenu.Append(DMCONVERSATIONBTN_ID, wxT("Open &DM Panel"));
	}

	int next_id = wxID_HIGHEST + 1;
	auto add_block_handler = [&](BLOCKTYPE type, bool unblock, twitcurlext_simple::CONNTYPE conntype) -> int {
		int id = next_id;
		next_id++;
		auto f = [=](wxCommandEvent &event) {
			if (!unblock) {
				wxString type_str;
				switch (type) {
					case BLOCKTYPE::BLOCK: type_str = wxT("block"); break;
					case BLOCKTYPE::MUTE: type_str = wxT("mute"); break;
					case BLOCKTYPE::NO_RT: type_str = wxT("disable retweets from"); break;
				}
				int result = ::wxMessageBox(wxString::Format(wxT("Are you sure that you want to %s @%s?"), type_str.c_str(), wxstrstd(u->GetUser().screen_name).c_str()),
						wxT("Confirm ") + type_str, wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION, this);
				if (result != wxYES) {
					return;
				}
			}

			if (u->udc_flags & UDC::FRIENDACT_IN_PROGRESS) {
				return;
			}

			std::unique_ptr<twitcurlext_simple> twit = twitcurlext_simple::make_new(acc, conntype);
			twit->extra_id = userid;
			twitcurlext::QueueAsyncExec(std::move(twit));
		};
		auto handler = evtbinder->MakeSharedEvtHandlerSC<wxCommandEvent>(f);
		evtbinder->BindEvtHandler(wxEVT_COMMAND_MENU_SELECTED, id, handler);
		return id;
	};

	bool isownacc = (acc && acc->usercont == u);
	if (!isownacc) {
		pmenu.AppendSeparator();
		wxMenuItem *block_item;
		wxMenuItem *mute_item;
		wxMenuItem *no_rt_item;
		if (acc->blocked_users.count(userid)) {
			block_item = pmenu.Append(add_block_handler(BLOCKTYPE::BLOCK, true, twitcurlext_simple::CONNTYPE::UNBLOCK), wxT("Unblock"));
		} else {
			block_item = pmenu.Append(add_block_handler(BLOCKTYPE::BLOCK, false, twitcurlext_simple::CONNTYPE::BLOCK), wxT("Block"));
		}
		if (acc->muted_users.count(userid)) {
			mute_item = pmenu.Append(add_block_handler(BLOCKTYPE::MUTE, true, twitcurlext_simple::CONNTYPE::UNMUTE), wxT("Unmute"));
		} else {
			mute_item = pmenu.Append(add_block_handler(BLOCKTYPE::MUTE, false, twitcurlext_simple::CONNTYPE::MUTE), wxT("Mute"));
		}
		if (acc->no_rt_users.count(userid)) {
			no_rt_item = pmenu.Append(add_block_handler(BLOCKTYPE::NO_RT, true, twitcurlext_simple::CONNTYPE::NO_RT_DESTROY), wxT("Enable retweets"));
		} else {
			no_rt_item = pmenu.Append(add_block_handler(BLOCKTYPE::NO_RT, false, twitcurlext_simple::CONNTYPE::NO_RT_CREATE), wxT("Disable retweets"));
		}
		if (u->udc_flags & UDC::FRIENDACT_IN_PROGRESS) {
			block_item->Enable(false);
			mute_item->Enable(false);
			no_rt_item->Enable(false);
		}
	}

	GenericPopupWrapper(this, &pmenu, btnrect.GetLeft(), btnrect.GetBottom());
}

void user_window::OnFollowBtn(wxCommandEvent &event) {
	std::shared_ptr<taccount> acc = acc_hint.lock();
	if (follow_btn_mode != FOLLOWBTNMODE::FBM_NONE && acc && acc->enabled && !(u->udc_flags & UDC::FRIENDACT_IN_PROGRESS)) {
		u->udc_flags |= UDC::FRIENDACT_IN_PROGRESS;
		followbtn->Enable(false);
		twitcurlext_simple::CONNTYPE type;
		if (follow_btn_mode == FOLLOWBTNMODE::FBM_FOLLOW) {
			type = twitcurlext_simple::CONNTYPE::FRIENDACTION_FOLLOW;
		} else {
			type = twitcurlext_simple::CONNTYPE::FRIENDACTION_UNFOLLOW;
		}
		std::unique_ptr<twitcurlext_simple> twit = twitcurlext_simple::make_new(acc, type);
		twit->extra_id = userid;
		twitcurlext::QueueAsyncExec(std::move(twit));
	}
}

void user_window::OnDMBtn(wxCommandEvent &event) {
	mainframe *win = GetMainframeAncestor(this);
	if (!win) {
		win = mainframelist.front();
	}
	if (win) {
		std::shared_ptr<taccount> acc = acc_hint.lock();
		if (acc) {
			win->tpw->accc->TrySetSel(acc.get());
		}
		win->tpw->SetDMTarget(u);
	}
}

void user_window::OnDMConversationBtn(wxCommandEvent &event) {
	mainframe *win = GetMainframeAncestor(this);
	if (!win) {
		win = mainframelist.front();
	}
	if (win) {
		auto tp = tpanel::MkTPanel("", "", TPF::DELETEONWINCLOSE, {}, { { TPFU::DMSET, u } });
		tp->MkTPanelWin(win, true);
	}
}

void user_window::OnNotesTextChange(wxCommandEvent &event) {
	// Lots of tiny updates is OK
	// Will be written to DB on next state flush/at termination
	std::string old_notes = std::move(u->GetUser().notes);
	u->GetUser().notes = stdstrwx(notes_txt->GetValue());
	u->GetUser().revision_number++;
	u->lastupdate_wrotetodb = 0;

	if (u->GetUser().notes.empty() != old_notes.empty()) {
		SetNotesTabTitle();
		SetNotebookMinSize();
		Layout();
		Fit();
	}
}

void user_window::SetNotesTabTitle() {
	for (size_t i = 0; i < nb->GetPageCount(); i++) {
		if (nb->GetPage(i) == notes_tab) {
			if (u->GetUser().notes.empty()) {
				nb->SetPageText(i, wxT("Notes"));
			} else {
				nb->SetPageText(i, wxT("Notes*"));
			}
			break;
		}
	}
}

// This makes sure that the window is wide enough that all of the notebook tabs fit
void user_window::SetNotebookMinSize() {
	int total_width = 0;
	for (size_t i = 0; i < nb->GetPageCount(); i++) {
		auto win = nb->GetPage(i);
		int x;
		win->GetTextExtent(nb->GetPageText(i), &x, nullptr);
		total_width += x;
	}
	total_width += (1 + nb->GetPageCount()) * 15; // This is a Magic Number™ which seems to be big enough to cover padding, borders, etc.
	SetSizeHints(total_width, 0);
}

user_window *user_window::GetWin(uint64_t userid_) {
	auto it = userwinmap.find(userid_);
	if (it != userwinmap.end()) {
		return it->second;
	} else {
		return 0;
	}
}

user_window *user_window::MkWin(uint64_t userid_, const std::shared_ptr<taccount> &acc_hint_) {
	user_window *cur = GetWin(userid_);
	if (cur) {
		cur->Show();
		cur->Raise();
		return cur;
	} else {
		return new user_window(userid_, acc_hint_);
	}
}

void user_window::RefreshAllAcc() {
	for (auto it = userwinmap.begin(); it != userwinmap.end(); ++it) {
		it->second->CheckAccHint();
		it->second->fill_accchoice();
		it->second->RefreshFollow();
	}
}

void user_window::RefreshAllFollow() {
	for (auto it = userwinmap.begin(); it != userwinmap.end(); ++it) {
		it->second->Refresh();
	}
}

void user_window::RefreshAll() {
	for (auto it = userwinmap.begin(); it != userwinmap.end(); ++it) {
		it->second->Refresh();
	}
}

void user_window::CloseAll() {
	for (auto it = userwinmap.begin(); it != userwinmap.end(); ++it) {
		it->second->Destroy();
	}
}

void user_window::CheckRefresh(uint64_t userid_, bool refreshimg, bool refresh_events) {
	user_window *cur = GetWin(userid_);
	if (cur) {
		cur->Refresh(refreshimg, refresh_events);
	}
}

void user_window::EventListUpdated(bool account_mode) {
	Freeze();
	bool new_tab = false;
	int target_cols = account_mode ? 4 : 3;
	if (events_grid) {
		events_grid->BeginBatch();
		if (target_cols < events_grid->GetNumberCols()) {
			events_grid->DeleteCols(target_cols, events_grid->GetNumberCols() - target_cols);
		} else if (target_cols > events_grid->GetNumberCols()) {
			events_grid->AppendCols(target_cols - events_grid->GetNumberCols());
		}
	} else {
		if (events.empty()) return;
		wxPanel *events_tab = new wxPanel(nb, wxID_ANY);
		wxSizer *events_sizer = new wxBoxSizer(wxVERTICAL);
		events_tab->SetSizer(events_sizer);
		wxString header_txt = wxT("User Events:");
		events_sizer->Add(new wxStaticText(events_tab, wxID_ANY, header_txt), 0, wxALL, 2);
		events_grid = new wxGrid(events_tab, EVENTS_GRID_ID);
		events_grid->EnableEditing(false);
		events_grid->DisableCellEditControl();
		events_grid->DisableDragColMove();
		events_grid->DisableDragColSize();
		events_grid->DisableDragGridSize();
		events_grid->DisableDragRowSize();
		events_grid->SetRowLabelSize(0);
		events_grid->SetColLabelSize(0);
		events_grid->EnableGridLines(false);
		events_grid->CreateGrid(0, target_cols);
		events_sizer->Add(events_grid, 1, wxALL | wxEXPAND | wxFIXED_MINSIZE, 2);
		nb->AddPage(events_tab, wxT("Events"));
		new_tab = true;
		events_grid->BeginBatch();
	}
	if (events.size() > (size_t) events_grid->GetNumberRows()) {
		events_grid->AppendRows(events.size() - events_grid->GetNumberRows());
	}
	for (size_t i = 0; i < events.size(); i++) {
		events_grid->SetCellValue(i, 0, cfg_strftime(events[i].eventtime));

		std::shared_ptr<taccount> acc = events[i].acc.lock();
		events_grid->SetCellValue(i, 1, acc ? acc->dispname : wxT("Unknown account"));

		wxString evt = wxT("Unknown Event");
		switch (events[i].type) {
			case DB_EVENTLOG_TYPE::FOLLOWED_ME:
				evt = wxT("Followed you");
				break;
			case DB_EVENTLOG_TYPE::FOLLOWED_ME_PENDING:
				evt = wxT("Followed you (pending)");
				break;
			case DB_EVENTLOG_TYPE::UNFOLLOWED_ME:
				evt = wxT("Unfollowed you");
				break;
			case DB_EVENTLOG_TYPE::I_FOLLOWED:
				evt = wxT("You followed");
				break;
			case DB_EVENTLOG_TYPE::I_FOLLOWED_PENDING:
				evt = wxT("You followed (pending)");
				break;
			case DB_EVENTLOG_TYPE::I_UNFOLLOWED:
				evt = wxT("You unfollowed");
				break;
		}
		events_grid->SetCellValue(i, 2, evt);
		if (account_mode) {
			events_grid->SetCellValue(i, 3, wxstrstd(user_screenname_log(events[i].obj_id)));
		}
	}

	events_grid->Layout();
	events_grid->AutoSize();
	events_grid->EndBatch();
	if (new_tab) {
		SetNotebookMinSize();
	}
	Thaw();
}

void user_window_timer::Notify() {
	user_window::RefreshAll();
}

std::weak_ptr<user_window_timer> user_window::uwt_common;

BEGIN_EVENT_TABLE(user_lookup_dlg, wxDialog)
EVT_TEXT_ENTER(wxID_FILE2, user_lookup_dlg::OnTCEnter)
END_EVENT_TABLE()

user_lookup_dlg::user_lookup_dlg(wxWindow *parent, int *type, wxString *value, std::shared_ptr<taccount> &acc)
		: wxDialog(parent, wxID_ANY, wxT("Enter user name or ID to look up"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE), curacc(acc) {

	const wxString opts[2] = {wxT("User screen name"), wxT("User numeric identifier")};
	*type = 0;
	wxRadioBox *rb = new wxRadioBox(this, wxID_FILE1, wxT("Type"), wxDefaultPosition, wxDefaultSize, 2, opts, 0, wxRA_SPECIFY_COLS, wxGenericValidator(type));
	*value = wxT("");
	wxTextCtrl *tc = new wxTextCtrl(this, wxID_FILE2, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER, wxGenericValidator(value));
	wxButton *okbtn = new wxButton(this, wxID_OK, wxT("OK"));
	wxButton *cancelbtn = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
	acc_choice *acd = new acc_choice(this, curacc, acc_choice::ACCCF::OKBTNCTRL | acc_choice::ACCCF::NOACCITEM);

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(acd, 0, wxALL, 2);
	vbox->Add(rb, 0, wxALL, 2);
	vbox->Add(tc, 0, wxALL | wxEXPAND, 2);
	wxBoxSizer *hboxfooter = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hboxfooter, 0, wxALL | wxEXPAND, 2);
	hboxfooter->AddStretchSpacer();
	hboxfooter->Add(okbtn, 0, wxALL | wxALIGN_BOTTOM | wxALIGN_RIGHT, 2);
	hboxfooter->Add(cancelbtn, 0, wxALL | wxALIGN_BOTTOM | wxALIGN_RIGHT, 2);

	tc->SetFocus();

	SetSizer(vbox);
	Fit();
}

void user_lookup_dlg::OnTCEnter(wxCommandEvent &event) {
	wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK);
	ProcessEvent(evt);
}

BEGIN_EVENT_TABLE(acc_choice, wxChoice)
	EVT_CHOICE(wxID_ANY, acc_choice::OnSelChange)
END_EVENT_TABLE()

acc_choice::acc_choice(wxWindow *parent, std::shared_ptr<taccount> &acc, flagwrapper<ACCCF> flags_, int winid, acc_choice_callback callbck, void *extra)
	: wxChoice(parent, winid, wxDefaultPosition, wxDefaultSize, 0, 0), curacc(acc), flags(flags_), fnptr(callbck), fnextra(extra) {
	if (!acc.get()) {
		for (auto &it : alist) {
			acc = it;
			if (it->enabled) break;
		}
	}
	fill_acc();
}

void acc_choice::fill_acc() {
	Clear();
	for (auto &it : alist) {
		wxString accname = it->dispname;
		wxString status = it->GetStatusString(true);
		if (status.size()) {
			accname += wxT(" [") + status + wxT("]");
		}
		int index = Append(accname, it.get());
		if (it.get() == curacc.get()) {
			SetSelection(index);
		}
	}
	if ((GetCount() == 0) && (flags & ACCCF::NOACCITEM)) {
		int index = Append(wxT("[No Accounts]"), (void *) 0);
		SetSelection(index);
	}
	UpdateSel();
}

void acc_choice::OnSelChange(wxCommandEvent &event) {
	UpdateSel();
}

void acc_choice::UpdateSel() {
	bool havegoodacc = false;
	bool haveanyacc = false;
	int selection = GetSelection();
	if (selection != wxNOT_FOUND) {
		taccount *accptr = static_cast<taccount *>(GetClientData(selection));
		for (auto &it : alist) {
			if (it.get() == accptr) {
				haveanyacc = true;
				curacc = it;
				if (it->enabled) {
					havegoodacc = true;
				}
				break;
			}
		}
	}
	if (!haveanyacc) {
		curacc.reset();
	}
	if (flags & ACCCF::OKBTNCTRL) {
		wxWindow *topparent = this;
		while (topparent) {
			if (topparent->IsTopLevel()) {
				break;
			} else {
				topparent = topparent->GetParent();
			}
		}
		wxWindow *okbtn = wxWindow::FindWindowById(wxID_OK, topparent);
		if (okbtn) {
			okbtn->Enable(havegoodacc);
		}
	}
	if (fnptr) {
		(*fnptr)(fnextra, this, havegoodacc);
	}
}

void acc_choice::TrySetSel(const taccount *tac) {
	for (unsigned int i = 0; i < GetCount(); i++) {
		if (GetClientData(i) == tac) {
			if ((int) i != GetSelection()) {
				SetSelection(i);
				UpdateSel();
			}
		}
	}
}

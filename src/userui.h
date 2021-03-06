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

#ifndef HGUARD_SRC_USERUI
#define HGUARD_SRC_USERUI

#include "univdefs.h"
#include "safe_observer_ptr.h"
#include "flags.h"
#include "ptr_types.h"
#include <wx/hyperlink.h>
#include <wx/radiobox.h>
#include <wx/valgen.h>
#include <wx/notebook.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <memory>
#include <forward_list>

struct acc_choice;
struct tpanelparentwin_usertweets;
struct tpanelparentwin_userproplisting;
struct userdatacontainer;
struct taccount;
struct user_window;
struct wxPanel;
struct wxGrid;
struct wxTextCtrl;
struct bindwxevt_win;

struct notebook_event_prehandler : public wxEvtHandler {
	void OnPageChange(wxNotebookEvent &event);
	wxNotebook *nb;
	user_window *uw;
	std::vector<tpanelparentwin_usertweets *> timeline_pane_list;
	std::vector<tpanelparentwin_userproplisting *> userlist_pane_list;

	DECLARE_EVENT_TABLE()
};

struct user_window_timer: public wxTimer {
	void Notify();
};

struct user_window: public wxDialog, public safe_observer_ptr_target {
	uint64_t userid;
	udc_ptr u;
	std::weak_ptr<taccount> acc_hint;

	wxFlexGridSizer *if_grid;

	wxStaticBitmap *usericon;
	wxStaticText *name;
	wxStaticText *screen_name;
	wxStaticText *name2;
	wxStaticText *screen_name2;
	wxStaticText *desc;
	wxStaticText *location;
	wxStaticText *isprotected;
	wxStaticText *isverified;
	wxStaticText *tweets;
	wxStaticText *followers;
	wxStaticText *follows;
	wxStaticText *faved;
	wxStaticText *createtime;
	wxStaticText *url_label;
	wxHyperlinkCtrl *url;
	wxHyperlinkCtrl *profileurl;
	wxStaticText *lastupdate;
	wxStaticText *id_str;
	wxFlexGridSizer *follow_grid;
	tpanelparentwin_usertweets *timeline_pane;
	tpanelparentwin_usertweets *fav_timeline_pane;
	tpanelparentwin_userproplisting *followers_pane;
	tpanelparentwin_userproplisting *friends_pane;
	tpanelparentwin_userproplisting *incoming_pane = nullptr;
	tpanelparentwin_userproplisting *outgoing_pane = nullptr;
	notebook_event_prehandler nb_prehndlr;
	wxNotebook *nb;
	int nb_tab_insertion_point = 0;
	wxTextCtrl *notes_txt;
	wxPanel *notes_tab;

	wxStaticText *ifollow;
	wxStaticText *followsme;
	wxStaticText *is_blocked;
	wxStaticText *is_muted;
	wxStaticText *is_no_rt;
	wxChoice *accchoice;

	wxButton *followbtn;
	wxButton *dmbtn;
	wxButton *morebtn;
	enum FOLLOWBTNMODE {
		FBM_NONE = 0, FBM_FOLLOW = 1, FBM_UNFOLLOW, FBM_REMOVE_PENDING,
	};
	FOLLOWBTNMODE follow_btn_mode;

	std::unique_ptr<bindwxevt_win> evtbinder;

	struct event_log_entry;
	std::vector<event_log_entry> events;
	wxGrid *events_grid = nullptr;

	enum {
		FOLLOWBTN_ID = 1,
		REFRESHBTN_ID,
		MOREBTN_ID,
		DMBTN_ID,
		DMCONVERSATIONBTN_ID,
		NOTESTXT_ID,
		EVENTS_GRID_ID,
	};

	user_window(uint64_t userid_, const std::shared_ptr<taccount> &acc_hint_);
	~user_window();
	void RefreshFollow(bool forcerefresh = false);
	void RefreshAccState();
	void Refresh(bool refreshimg = false, bool refresh_events = false);
	void CheckAccHint();
	void fill_accchoice();
	void OnClose(wxCloseEvent &event);
	void OnSelChange(wxCommandEvent &event);
	void OnRefreshBtn(wxCommandEvent &event);
	void OnMoreBtn(wxCommandEvent &event);
	void OnFollowBtn(wxCommandEvent &event);
	void OnDMBtn(wxCommandEvent &event);
	void OnDMConversationBtn(wxCommandEvent &event);
	void OnNotesTextChange(wxCommandEvent &event);
	void SetNotebookMinSize();
	void SetNotesTabTitle();
	static user_window *MkWin(uint64_t userid_, const std::shared_ptr<taccount> &acc_hint_);
	static user_window *GetWin(uint64_t userid_);
	static void CheckRefresh(uint64_t userid_, bool refreshimg = false, bool refresh_events = false);
	static void RefreshAllFollow();
	static void RefreshAllAcc();
	static void RefreshAll();
	static void CloseAll();

	std::shared_ptr<user_window_timer> uwt;
	static std::weak_ptr<user_window_timer> uwt_common;

	private:
	void EventListUpdated(bool account_mode);

	public:
	DECLARE_EVENT_TABLE()
};

typedef void (*acc_choice_callback)(void *, acc_choice *, bool);

struct acc_choice: public wxChoice {
	std::shared_ptr<taccount> &curacc;

	enum class ACCCF {
		OKBTNCTRL     = 1<<0,
		NOACCITEM     = 1<<1,
	};
	flagwrapper<ACCCF> flags;
	acc_choice_callback fnptr;
	void *fnextra;

	acc_choice(wxWindow *parent, std::shared_ptr<taccount> &acc, flagwrapper<ACCCF> flags_, int winid = wxID_ANY, acc_choice_callback callbck = nullptr, void *extra = nullptr);
	void UpdateSel();
	void OnSelChange(wxCommandEvent &event);
	void fill_acc();
	void TrySetSel(const taccount *tac);

	DECLARE_EVENT_TABLE()
};
template<> struct enum_traits<acc_choice::ACCCF> { static constexpr bool flags = true; };

struct user_lookup_dlg: public wxDialog {
	std::shared_ptr<taccount> &curacc;

	user_lookup_dlg(wxWindow *parent, int *type, wxString *value, std::shared_ptr<taccount> &acc);
	void OnTCEnter(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif

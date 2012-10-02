//  retcon
//
//  WEBSITE: http://retcon.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
//
//  2012 - j.g.rennison@gmail.com
//==========================================================================

#if !(defined(RCS_GTKSOCKMODE) || defined(RCS_WSAASYNCSELMODE) || defined(RCS_POLLTHREADMODE) || defined(RCS_SIGNALMODE))
	#if defined(__WXGTK__)
		#define RCS_GTKSOCKMODE
	#elif defined(__WINDOWS__)
		#define RCS_WSAASYNCSELMODE
	#else
		#define RCS_POLLTHREADMODE
	#endif
#endif

#ifdef RCS_GTKSOCKMODE
	#include <glib.h>
#endif

//arrange in order of increasing severity
typedef enum {
	MCC_RETRY=0,
	MCC_FAILED,
} MCC_HTTPERRTYPE;

enum {	MCCT_RETRY=wxID_HIGHEST+1,
};

enum {
	MCF_NOTIMEOUT		= 1<<0,
	MCF_IN_RETRY_QUEUE	= 1<<1,
	MCF_RETRY_NOW_ON_SUCCESS= 1<<2,
};

struct mcurlconn : public wxEvtHandler {
	void NotifyDone(CURL *easy, CURLcode res);
	void HandleError(CURL *easy, long httpcode, CURLcode res);
	void StandbyTidy();
	unsigned int errorcount;
	unsigned int mcflags;
	mcurlconn() : errorcount(0), mcflags(0) {}
	virtual ~mcurlconn();

	virtual void NotifyDoneSuccess(CURL *easy, CURLcode res)=0;
	virtual void DoRetry()=0;
	virtual void HandleFailure(long httpcode, CURLcode res)=0;
	virtual void KillConn();
	virtual MCC_HTTPERRTYPE CheckHTTPErrType(long httpcode);
	virtual CURL *GenGetCurlHandle()=0;
	virtual wxString GetConnTypeName() { return wxT(""); }

	DECLARE_EVENT_TABLE()
};

template <typename C> struct connpool {
	void ClearAllConns();
	C *GetConn();
	~connpool();
	void Standby(C *obj);

	std::stack<C *> idlestack;
	std::unordered_set<C *> activeset;
};

struct dlconn : public mcurlconn {
	CURL* curlHandle;
	std::string url;
	std::string data;

	static int curlCallback(char* data, size_t size, size_t nmemb, dlconn *obj);
	dlconn();
	void Init(const std::string &url_);
	void Reset();
	~dlconn();
	CURL *GenGetCurlHandle() { return curlHandle; }
};

struct profileimgdlconn : public dlconn {
	std::shared_ptr<userdatacontainer> user;
	static connpool<profileimgdlconn> cp;

	void Init(const std::string &imgurl_, const std::shared_ptr<userdatacontainer> &user_);

	void NotifyDoneSuccess(CURL *easy, CURLcode res);
	void Reset();
	void DoRetry();
	void HandleFailure(long httpcode, CURLcode res);
	static profileimgdlconn *GetConn(const std::string &imgurl_, const std::shared_ptr<userdatacontainer> &user_);
	virtual wxString GetConnTypeName();
};

enum {
	MIDC_FULLIMG			= 1<<0,
	MIDC_THUMBIMG			= 1<<1,
	MIDC_REDRAW_TWEETS		= 1<<2,
	MIDC_OPPORTUNIST_THUMB		= 1<<3,
	MIDC_OPPORTUNIST_REDRAW_TWEETS	= 1<<4,
};

struct mediaimgdlconn : public dlconn {
	media_id_type media_id;
	unsigned int flags;

	void Init(const std::string &imgurl_, media_id_type media_id_, unsigned int flags_=0);
	mediaimgdlconn(const std::string &imgurl_, media_id_type media_id_, unsigned int flags_=0) { Init(imgurl_, media_id_, flags_); }

	void NotifyDoneSuccess(CURL *easy, CURLcode res);
	void Reset();
	void DoRetry();
	void HandleFailure(long httpcode, CURLcode res);
	virtual wxString GetConnTypeName();
};

struct sockettimeout : public wxTimer {
	socketmanager &sm;
	sockettimeout(socketmanager &sm_) : sm(sm_) {};
	void Notify();
};

#ifdef RCS_POLLTHREADMODE

typedef enum {
	SPM_FDCHANGE=1,
	SPM_ENABLE,
	SPM_QUIT,
} SPM_TYPE;

struct socketpollmessage {
	SPM_TYPE type;
	int fd;
	int events;
};

struct socketpollthread : public wxThread {
	int pipefd;

	socketpollthread() : wxThread(wxTHREAD_DETACHED) { }
	wxThread::ExitCode Entry();
};

#endif

#if defined(RCS_POLLTHREADMODE) || defined(RCS_SIGNALMODE)

DECLARE_EVENT_TYPE(wxextSOCK_NOTIFY, -1)

struct wxextSocketNotifyEvent : public wxEvent {
	wxextSocketNotifyEvent( int id=0 );
	wxextSocketNotifyEvent( const wxextSocketNotifyEvent &src );
	wxEvent *Clone() const;

	int fd;
	int curlbitmask;
	bool reenable;
};

typedef void (wxEvtHandler::*wxextSocketNotifyEventFunction)(wxextSocketNotifyEvent&);

#define EVT_EXTSOCKETNOTIFY(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( wxextSOCK_NOTIFY, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) \
    wxStaticCastEvent( wxextSocketNotifyEventFunction, & fn ), (wxObject *) NULL ),

#endif

struct socketmanager : public wxEvtHandler {
	socketmanager();
	~socketmanager();
	bool AddConn(CURL* ch, mcurlconn *cs);
	bool AddConn(twitcurlext &cs);
	void RemoveConn(CURL* ch);
	void RegisterSockInterest(CURL *e, curl_socket_t s, int what);
	void NotifySockEvent(curl_socket_t sockfd, int ev_bitmask);
	#if defined(RCS_POLLTHREADMODE) || defined(RCS_SIGNALMODE)
	void NotifySockEventCmd(wxextSocketNotifyEvent &event);
	#endif
	void InitMultiIOHandler();
	void DeInitMultiIOHandler();
	void InitMultiIOHandlerCommon();
	void DeInitMultiIOHandlerCommon();
	void RetryConn(mcurlconn *cs);
	void RetryConnNow();
	void RetryConnLater();
	void RetryNotify(wxTimerEvent& event);
	void UnregisterRetryConn(mcurlconn *cs);

	bool MultiIOHandlerInited;
	CURLM *curlmulti;
	sockettimeout *st;
	int curnumsocks;
	#ifdef RCS_WSAASYNCSELMODE
	HWND wind;
	#endif
	#ifdef RCS_POLLTHREADMODE
	int pipefd;
	#endif
	#ifdef RCS_GTKSOCKMODE
	GSource *gs;
	unsigned int source_id;
	std::map<curl_socket_t,GPollFD> sockpollmap;
	#endif
	std::forward_list<std::pair<CURL*, mcurlconn *> > connlist;
	std::deque<mcurlconn *> retry_conns;
	wxTimer *retry;

	DECLARE_EVENT_TABLE()
};

void SetCurlHandleVerboseState(CURL *easy, bool verbose);
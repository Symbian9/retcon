//arrange in order of increasing severity
typedef enum {
	MCC_RETRY=0,
	MCC_FAILED,
} MCC_HTTPERRTYPE;

enum {	MCCT_RETRY=wxID_HIGHEST+1,
};

enum {
	MCF_NOTIMEOUT	= 1<<0,
};

struct mcurlconn : public wxEvtHandler {
	void NotifyDone(CURL *easy, CURLcode res);
	void HandleError(CURL *easy, long httpcode, CURLcode res);
	void setlog(FILE *fs, bool verbose);
	void RetryNotify(wxTimerEvent& event);
	void StandbyTidy();
	wxTimer *tm;
	unsigned int errorcount;
	unsigned int mcflags;
	mcurlconn() : tm(0), errorcount(0), mcflags(0) {}

	virtual void NotifyDoneSuccess(CURL *easy, CURLcode res)=0;
	virtual void DoRetry()=0;
	virtual void HandleFailure()=0;
	virtual void KillConn();
	virtual MCC_HTTPERRTYPE CheckHTTPErrType(long httpcode) { return MCC_RETRY; }
	virtual CURL *GenGetCurlHandle()=0;

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

struct imgdlconn : public mcurlconn {
	CURL* curlHandle;
	std::string imgurl;
	std::shared_ptr<userdatacontainer> user;
	std::string imgdata;

	void NotifyDoneSuccess(CURL *easy, CURLcode res);
	static int curlCallback(char* data, size_t size, size_t nmemb, imgdlconn *obj);
	imgdlconn();
	void Init(std::string &imgurl_, std::shared_ptr<userdatacontainer> user_);
	~imgdlconn();
	CURL *GenGetCurlHandle() { return curlHandle; }
	void Reset();
	void DoRetry();
	void HandleFailure();

	static imgdlconn *GetConn(std::string &imgurl_, std::shared_ptr<userdatacontainer> user_);
	static connpool<imgdlconn> cp;

	DECLARE_EVENT_TABLE()
};

struct sockettimeout : public wxTimer {
	socketmanager &sm;
	sockettimeout(socketmanager &sm_) : sm(sm_) {};
	void Notify();
};

struct socketmanager : public wxEvtHandler {
	socketmanager();
	~socketmanager();
	bool AddConn(CURL* ch, mcurlconn *cs);
	bool AddConn(twitcurlext &cs);
	void RemoveConn(CURL* ch);
	void RegisterSockInterest(CURL *e, curl_socket_t s, int what);
	void NotifySockEvent(curl_socket_t sockfd, int ev_bitmask);
	void NotifySockEventCmd(wxCommandEvent &event);
	void InitMultiIOHandler();
	void DeInitMultiIOHandler();
	bool MultiIOHandlerInited;

	CURLM *curlmulti;
	sockettimeout st;
	int curnumsocks;
	#ifdef __WINDOWS__
	HWND wind;
	#endif
	FILE *loghandle;

	DECLARE_EVENT_TABLE()
};

extern socketmanager sm;

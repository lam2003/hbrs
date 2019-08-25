#include "system/mpp.h"
#include "system/vm.h"
#include "common/logger.h"
#include "common/http_server.h"
#include "common/switch.h"
#include "common/json.h"

#include "model/record_req.h"
#include "model/local_live_req.h"
#include "model/remote_live_req.h"
#include "model/change_pc_capture_req.h"
#include "model/change_main_sceen_req.h"
#include "model/change_display_sceen_req.h"
#include "model/switch_req.h"
#include "model/change_video_req.h"

using namespace rs;

#define CHECK_ERROR(a)                                                                  \
	if (KSuccess != a)                                                                  \
	{                                                                                   \
		log_e("error:%s", make_error_code(static_cast<err_code>(a)).message().c_str()); \
		return a;                                                                       \
	}

static VideoManager g_VideoManager;
static HttpServer g_HttpServer;
static Switch g_Switch;

static const char *g_Opts = "c:i:p:";

struct option g_LongOpts[] = {
	{"config", 1, NULL, 'c'},
	{"http listen ip", 2, NULL, 'i'},
	{"http listen port", 3, NULL, 'p'},
	{0, 0, 0, 0}};
static bool g_Run = true;

static void SignalHandler(int signo)
{
	if (signo == SIGINT)
	{
		log_w("recive signal SIGINT,going to shutdown");
		g_Run = false;
	}
	else if (signo == SIGPIPE)
	{
		log_w("receive signal SIGPIPE");
	}
}

template <typename T>
bool CheckReq(evhttp_request *req, Json::Value &root)
{
	std::string str = HttpServer::GetRequestData(req);
	log_d("request body:%s", str.c_str());

	if (JsonUtils::toJson(str, root) != KSuccess)
	{
		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
		log_w("parse json root failed");
		return false;
	}

	if (!T::IsOk(root))
	{
		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
		log_w("check json format failed");
		return false;
	}
	return true;
}

void ResponseOK(evhttp_request *req)
{
	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
}

static void StartLocalLiveHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<LocalLiveReq>(req, root))
		return;

	LocalLiveReq live_req;
	live_req = root;

	g_VideoManager.CloseLocalLive();
	g_VideoManager.StartLocalLive(live_req.local_lives);
	ResponseOK(req);
}

static void StopLocalLiveHandler(evhttp_request *req, void *arg)
{
	g_VideoManager.CloseLocalLive();
	ResponseOK(req);
}

static void StartRemoteLiveHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<RemoteLiveReq>(req, root))
		return;

	RemoteLiveReq live_req;
	live_req = root;

	g_VideoManager.CloseRemoteLive();
	g_VideoManager.StartRemoteLive(live_req.remote_live);
	ResponseOK(req);
}

static void StopRemoteLiveHandler(evhttp_request *req, void *arg)
{
	g_VideoManager.CloseRemoteLive();
	ResponseOK(req);
}

static void StartRecordHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<RecordReq>(req, root))
		return;

	RecordReq record_req;
	record_req = root;

	g_VideoManager.CloseRecord();
	g_VideoManager.StartRecord(record_req.records);
	ResponseOK(req);
}

static void SwitchHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<SwitchReq>(req, root))
		return;

	SwitchReq switch_req;
	switch_req = root;

	g_VideoManager.OnSwitchEvent(switch_req.scene);
	ResponseOK(req);
}

static void StopRecordHandler(evhttp_request *req, void *arg)
{
	g_VideoManager.CloseRecord();
	ResponseOK(req);
}

static void ChangeMainScreenHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeMainScreenReq>(req, root))
		return;

	ChangeMainScreenReq change_main_screen_req;
	change_main_screen_req = root;

	if (Config::IsResourceMode(change_main_screen_req.scene.mode) != CONFIG->IsResourceMode())
	{
		g_VideoManager.CloseRecord();
		g_VideoManager.CloseRemoteLive();
		g_VideoManager.CloseLocalLive();
		g_VideoManager.CloseVideoEncode();
		g_VideoManager.CloseMainScreen();

		g_VideoManager.StartMainScreen(change_main_screen_req.scene);
		g_VideoManager.StartVideoEncode(CONFIG->video_);
		g_VideoManager.StartLocalLive(CONFIG->local_lives_);
		g_VideoManager.StartRemoteLive(CONFIG->remote_live_);
	}
	else
	{
		g_VideoManager.CloseMainScreen();
		g_VideoManager.StartMainScreen(change_main_screen_req.scene);
	}
	ResponseOK(req);
}

static void ChangePCCaptureModeHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangePCCaptureReq>(req, root))
		return;

	ChangePCCaptureReq change_pc_capture_req;
	change_pc_capture_req = root;

	g_VideoManager.ChangePCCaputreMode(change_pc_capture_req.adv7842);
	ResponseOK(req);
}

static void ChangeDisplayScreenHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeDisplayScreenReq>(req, root))
		return;

	ChangeDisplayScreenReq change_display_screen_req;
	change_display_screen_req = root;

	g_VideoManager.CloseDisplayScreen();
	g_VideoManager.StartDisplayScreen(change_display_screen_req.display);

	ResponseOK(req);
}

static void ChangeVideoHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeVideoReq>(req, root))
		return;
	ChangeVideoReq change_video_req;
	change_video_req = root;

	g_VideoManager.CloseRecord();
	g_VideoManager.CloseLocalLive();
	g_VideoManager.CloseLocalLive();
	g_VideoManager.CloseVideoEncode();
	g_VideoManager.StartVideoEncode(change_video_req.video);
	g_VideoManager.StartLocalLive(CONFIG->local_lives_);
	g_VideoManager.StartRemoteLive(CONFIG->remote_live_);
	ResponseOK(req);
}

int32_t main(int32_t argc, char **argv)
{
	int ret;

	ConfigLogger();

	signal(SIGINT, SignalHandler);
	signal(SIGPIPE, SignalHandler);

	const char *ip = "0.0.0.0";
	int port = 8081;
	bool got_config_file = false;
	int opt;
	while ((opt = getopt_long(argc, argv, g_Opts, g_LongOpts, NULL)) != -1)
	{
		switch (opt)
		{
		case 'c':
		{
			ret = CONFIG->Initialize(optarg);
			CHECK_ERROR(ret);
			got_config_file = true;
			break;
		}
		case 'i':
		{
			ip = optarg;
			break;
		}
		case 'p':
		{
			port = atoi(optarg);
			break;
		}
		default:
		{
			log_e("unknow argument:%c", opt);
			return 0;
		}
		}
	}

	if (!got_config_file)
	{
		log_w("Usage:%s -c [conf_file_path] -i <http_listen_ip> -p <http_listen_port>", argv[0]);
		usleep(100000); //100ms
		return 0;
	}

	ret = MPPSystem::Instance()->Initialize();
	CHECK_ERROR(ret);

	g_VideoManager.Initialize();

	event_base *base = event_base_new();
	g_Switch.Initialize(base);
	g_Switch.SetEventListener(&g_VideoManager);
	g_HttpServer.Initialize(ip, port, base);
	g_HttpServer.RegisterURI("/start_local_live", StartLocalLiveHandler, nullptr);
	g_HttpServer.RegisterURI("/stop_local_live", StopLocalLiveHandler, nullptr);
	g_HttpServer.RegisterURI("/start_remote_live", StartRemoteLiveHandler, nullptr);
	g_HttpServer.RegisterURI("/stop_remote_live", StopRemoteLiveHandler, nullptr);
	g_HttpServer.RegisterURI("/start_record", StartRecordHandler, nullptr);
	g_HttpServer.RegisterURI("/stop_record", StopRecordHandler, nullptr);
	g_HttpServer.RegisterURI("/switch", SwitchHandler, nullptr);
	g_HttpServer.RegisterURI("/change_main_screen", ChangeMainScreenHandler, nullptr);
	g_HttpServer.RegisterURI("/change_pc_capture_mode", ChangePCCaptureModeHandler, nullptr);
	g_HttpServer.RegisterURI("/change_display_screen", ChangeDisplayScreenHandler, nullptr);
	g_HttpServer.RegisterURI("/change_video", ChangeVideoHandler, nullptr);

	while (g_Run)
	{
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500000; //500ms
		event_base_loopexit(base, &tv);
		event_base_dispatch(base);
	}

	g_Switch.Close();
	g_HttpServer.Close();
	event_base_free(base);

	g_VideoManager.Close();
	return 0;
}

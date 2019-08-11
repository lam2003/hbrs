#include "system/mpp.h"
#include "system/vm.h"
#include "common/logger.h"
#include "common/http_server.h"
#include "common/json.h"

#include "model/resource_record_req.h"
#include "model/local_live_req.h"
#include "model/remote_live_req.h"
#include "model/change_pc_capture_req.h"
#include "model/change_main_scene_req.h"

using namespace rs;

#define CHECK_ERROR(a)                                                                  \
	if (KSuccess != a)                                                                  \
	{                                                                                   \
		log_e("error:%s", make_error_code(static_cast<err_code>(a)).message().c_str()); \
		return a;                                                                       \
	}

static const char *g_Opts = "c:";
struct option g_LongOpts[] = {
	{"config", 1, NULL, 'c'},
	{0, 0, 0, 0}};

static RS_SCENE g_CurMainScene = PC_CAPTURE;

static bool g_Run = true;
static bool g_LiveStart = false;
static bool g_RemoteLiveStart = false;
static bool g_RecordStart = false;
static bool g_MainScreenStart = false;

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

static VideoManager g_VideoManager;

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

static void StopLiveHandler(evhttp_request *req, void *arg)
{
	g_VideoManager.CloseLocalLive();
	ResponseOK(req);
}

// static void StartRemoteLiveHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!RemoteLiveReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}

// 	RemoteLiveReq remote_live_req;
// 	remote_live_req = root;

// 	CloseRemoteLive();
// 	ret = StartRemoteLive(remote_live_req.remote_live.live);
// 	if (ret != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start remote live failed\"}");
// 		log_w("start remote live failed");
// 		return;
// 	}

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->remote_live_ = remote_live_req.remote_live;
// 	Config::Instance()->WriteToFile();
// }

// static void StopRemoteLiveHandler(evhttp_request *req, void *arg)
// {
// 	CloseRemoteLive();
// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->remote_live_.live.url = "";
// 	Config::Instance()->WriteToFile();
// }

// static void ChangePCCaptureHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!ChangePCCaptureReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}
// 	ChangePCCaptureReq change_pc_capture_req;
// 	change_pc_capture_req = root;

// 	Config::Instance()->system_.pc_capture_mode = change_pc_capture_req.mode;
// 	// ret = SigDetect::Instance()->SetPCCaptureMode(change_pc_capture_req.mode);
// 	// if (ret != KSuccess)
// 	// {
// 	// 	HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"change pc capture mode failed\"}");
// 	// 	log_w("change pc capture mode failed");
// 	// 	return;
// 	// }

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->WriteToFile();
// }

// static void ChangeMainScreenHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!ChangeMainScreenReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}

// 	ChangeMainScreenReq change_main_screen_req;
// 	change_main_screen_req = root;

// 	CloseRecord();
// 	CloseLive();
// 	CloseRemoteLive();
// 	CloseMainScreen();

// 	if (Config::IsResourceMode(change_main_screen_req.scene.mode) != Config::Instance()->IsResourceMode())
// 	{
// 		log_d("need to restart video encode module");
// 		CloseVideoEncode();
// 		Config::Instance()->scene_.mode = change_main_screen_req.scene.mode;
// 		ret = StartVideoEncode();
// 		if (ret != KSuccess)
// 		{
// 			HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start video encode failed\"}");
// 			log_w("start video encode failed");
// 			return;
// 		}
// 	}

// 	Config::Instance()->scene_ = change_main_screen_req.scene;

// 	ret = StartMainScreen();
// 	if (ret != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start main screen failed\"}");
// 		log_w("start main screen failed");
// 		return;
// 	}

// 	if (!Config::Instance()->local_lives_.lives.empty())
// 	{
// 		ret = StartLive(Config::Instance()->local_lives_.lives);
// 		if (ret != KSuccess)
// 		{
// 			HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start local live failed\"}");
// 			log_w("start local live failed");
// 			return;
// 		}
// 	}

// 	if (Config::Instance()->remote_live_.live.url != "")
// 	{
// 		ret = StartRemoteLive(Config::Instance()->remote_live_.live);
// 		if (ret != KSuccess)
// 		{
// 			HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start remote live failed\"}");
// 			log_w("start remote live failed");
// 			return;
// 		}
// 	}

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");
// 	Config::Instance()->WriteToFile();
// }

int32_t main(int32_t argc, char **argv)
{
	int ret;

	ConfigLogger();

	signal(SIGINT, SignalHandler);
	signal(SIGPIPE, SignalHandler);

	bool got_config_file = false;
	int opt;
	while ((opt = getopt_long(argc, argv, g_Opts, g_LongOpts, NULL)) != -1)
	{
		switch (opt)
		{
		case 'c':
		{
			log_w("using config file %s", optarg);
			ret = Config::Instance()->Initialize(optarg);
			CHECK_ERROR(ret);
			got_config_file = true;
			break;
		}
		default:
		{
			log_w("unknow argument:%c", opt);
			break;
		}
		}
	}

	if (!got_config_file)
	{
		log_w("Usage:%s -c [conf_file_path]", argv[0]);
		//休眠让日志有足够的时间输出
		usleep(100000); //100ms
		return 0;
	}

	ret = MPPSystem::Instance()->Initialize();
	CHECK_ERROR(ret);

	// VideoManager vm;
	g_VideoManager.Initialize();

	HttpServer http_server;
	http_server.Initialize("0.0.0.0", 8081);
	http_server.RegisterURI("/start_local_live", StartLocalLiveHandler, nullptr);
	http_server.RegisterURI("/stop_local_live", StartLocalLiveHandler, nullptr);
	// 	http_server.RegisterURI("/start_local_live", StartLiveHandler, nullptr);
	// 	http_server.RegisterURI("/stop_local_live", StopLiveHandler, nullptr);
	// 	http_server.RegisterURI("/start_remote_live", StartRemoteLiveHandler, nullptr);
	// 	http_server.RegisterURI("/stop_remote_live", StopRemoteLiveHandler, nullptr);
	// 	http_server.RegisterURI("/change_pc_capture", ChangePCCaptureHandler, nullptr);
	// 	http_server.RegisterURI("/change_main_screen", ChangeMainScreenHandler, nullptr);

	// #if 0
	// 	ChangeMainScreenReq test_req;
	// 	test_req.mode = Config::Instance()->scene_.mode;
	// 	test_req.mapping = Config::Instance()->scene_.mapping;

	// 	Json::Value test_json = test_req;
	// 	std::string test_str = JsonUtils::toStr(test_json);
	// 		printf("test_req:%s\n", test_str.c_str());

	// 	Json::Value test_json2;
	// 	if (JsonUtils::toJson(test_str, test_json2) == 0)
	// 	{
	// 		if (ChangeMainScreenReq::IsOk(test_json2))
	// 		{
	// 			std::string test_str2 = JsonUtils::toStr(test_json2);
	// 			printf("#####:%s\n",test_str2.c_str());
	// 		}
	// 	}
	// #endif

	// 	Config::Instance()->WriteToFile();
	while (g_Run)
		http_server.Dispatch();

	g_VideoManager.Close();

	return 0;
}

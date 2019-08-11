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

static VideoManager g_VideoManager;

static const char *g_Opts = "c:";

struct option g_LongOpts[] = {
	{"config", 1, NULL, 'c'},
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

static void StartResourceRecordHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ResourceRecordReq>(req, root))
		return;

	ResourceRecordReq record_req;
	record_req = root;

	g_VideoManager.CloseResourceRecord();
	g_VideoManager.StartResourceRecord(record_req.records);
	ResponseOK(req);
}

static void StopResourceRecordHandler(evhttp_request *req, void *arg)
{
	g_VideoManager.CloseResourceRecord();
	ResponseOK(req);
}

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
			ret = CONFIG->Initialize(optarg);
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

	g_VideoManager.Initialize();

	HttpServer http_server;
	http_server.Initialize("0.0.0.0", 8081);
	http_server.RegisterURI("/start_local_live", StartLocalLiveHandler, nullptr);
	http_server.RegisterURI("/stop_local_live", StopLocalLiveHandler, nullptr);
	http_server.RegisterURI("/start_resource_record", StartResourceRecordHandler, nullptr);
	http_server.RegisterURI("/stop_resource_record", StopResourceRecordHandler, nullptr);
	// 	http_server.RegisterURI("/start_remote_live", StartRemoteLiveHandler, nullptr);
	// 	http_server.RegisterURI("/stop_remote_live", StopRemoteLiveHandler, nullptr);
	// 	http_server.RegisterURI("/change_pc_capture", ChangePCCaptureHandler, nullptr);
	// 	http_server.RegisterURI("/change_main_screen", ChangeMainScreenHandler, nullptr);

	while (g_Run)
		http_server.Dispatch();

	g_VideoManager.Close();
	return 0;
}

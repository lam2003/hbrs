#include "system/mpp.h"
#include "system/vm.h"

#include "common/rtc.h"
#include "common/http_server.h"
#include "common/http_client.h"
#include "common/switch.h"
#include "common/json.h"
#include "common/bind_cpu.h"
#include "common/system.h"

#include "model/record_req.h"
#include "model/local_live_req.h"
#include "model/remote_live_req.h"
#include "model/change_pc_capture_req.h"
#include "model/change_main_sceen_req.h"
#include "model/change_display_sceen_req.h"
#include "model/switch_req.h"
#include "model/change_video_req.h"
#include "model/camera_control_req.h"
#include "model/change_switch_command_req.h"
#include "model/change_record_mode_req.h"
#include "model/change_osd_ts_req.h"
#include "model/change_osd_req.h"

using namespace rs;

static std::shared_ptr<AVManager> g_AVManager = std::make_shared<AVManager>();
static std::shared_ptr<HttpServer> g_HttpServer = std::make_shared<HttpServer>();
static std::shared_ptr<SerialManager> g_SerialManager = std::make_shared<SerialManager>();
static std::string g_Opt = "";
static event_base *g_Base = event_base_new();

static const char *g_Opts = "c:i:p:";

struct option g_LongOpts[] = {
	{"config", 1, NULL, 'c'},
	{"http listen ip", 2, NULL, 'i'},
	{"http listen port", 3, NULL, 'p'},
	{0, 0, 0, 0}};
static bool g_Run = true;

static void SignalHandler(int signo)
{
	if ((signo == SIGINT || signo == SIGTERM))
	{
		log_w("recive signal SIGINT,going to shutdown");
		if (!g_Run)
			return;
		g_Run = false;
		event_base_loopexit(g_Base, nullptr);
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

	g_AVManager->CloseLocalLive();
	g_AVManager->StartLocalLive(live_req.local_lives);
	ResponseOK(req);
}

static void StopLocalLiveHandler(evhttp_request *req, void *arg)
{
	g_AVManager->CloseLocalLive();
	ResponseOK(req);
}

static void StartRemoteLiveHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<RemoteLiveReq>(req, root))
		return;

	RemoteLiveReq live_req;
	live_req = root;

	g_AVManager->CloseRemoteLive();
	g_AVManager->StartRemoteLive(live_req.remote_live);
	ResponseOK(req);
}

static void StopRemoteLiveHandler(evhttp_request *req, void *arg)
{
	g_AVManager->CloseRemoteLive();
	ResponseOK(req);
}

static void StartRecordHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<RecordReq>(req, root))
		return;

	RecordReq record_req;
	record_req = root;

	g_AVManager->CloseRecord();
	g_AVManager->StartRecord(record_req.records);
	ResponseOK(req);
}

static void SwitchHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<SwitchReq>(req, root))
		return;

	SwitchReq switch_req;
	switch_req = root;

	g_AVManager->OnSwitchEvent(switch_req.scene);
	ResponseOK(req);
}

static void StopRecordHandler(evhttp_request *req, void *arg)
{
	g_AVManager->CloseRecord();
	ResponseOK(req);
}

static void ChangeMainScreenHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeMainScreenReq>(req, root))
		return;

	ChangeMainScreenReq change_main_screen_req;
	change_main_screen_req = root;
	g_AVManager->CloseMainScreen();
	g_AVManager->StartMainScreen(change_main_screen_req.scene);
	ResponseOK(req);
}

static void ChangeRecordModeHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeRecordModeReq>(req, root))
		return;

	ChangeRecordModeReq change_record_mode_req;
	change_record_mode_req = root;
	CONFIG->record_mode_ = change_record_mode_req.record_mode;
	g_AVManager->CloseRecord();
	g_AVManager->CloseRemoteLive();
	g_AVManager->CloseLocalLive();
	g_AVManager->CloseVideoEncode();
	g_AVManager->StartVideoEncode(CONFIG->video_);
	ResponseOK(req);
}

static void ChangePCCaptureModeHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangePCCaptureReq>(req, root))
		return;

	ChangePCCaptureReq change_pc_capture_req;
	change_pc_capture_req = root;

	g_AVManager->ChangePCCaputreMode(change_pc_capture_req.adv7842);
	ResponseOK(req);
}

static void ChangeDisplayScreenHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeDisplayScreenReq>(req, root))
		return;

	ChangeDisplayScreenReq change_display_screen_req;
	change_display_screen_req = root;

	g_AVManager->CloseDisplayScreen();
	g_AVManager->StartDisplayScreen(change_display_screen_req.display);

	ResponseOK(req);
}

static void ChangeVideoHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeVideoReq>(req, root))
		return;
	ChangeVideoReq change_video_req;
	change_video_req = root;

	g_AVManager->CloseRecord();
	g_AVManager->CloseLocalLive();
	g_AVManager->CloseLocalLive();
	g_AVManager->CloseVideoEncode();
	g_AVManager->StartVideoEncode(change_video_req.video);
	ResponseOK(req);
}

static void SaveTimeHandler(evhttp_request *req, void *arg)
{
	RTC::SaveTime();
	ResponseOK(req);
}

static void ShutDownHandler(evhttp_request *req, void *arg)
{
	log_d("going to shutdown...");
	g_Opt = "shutdown";
	ResponseOK(req);
	SignalHandler(SIGINT);
}

static void ReBootHandler(evhttp_request *req, void *arg)
{
	log_d("going to reboot...");
	g_Opt = "reboot";
	ResponseOK(req);
	SignalHandler(SIGINT);
}

static void CameraControlHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<CameraControlReq>(req, root))
		return;
	CameraControlReq camera_control_req;
	camera_control_req = root;
	g_SerialManager->CameraControl(camera_control_req.camera_addr, camera_control_req.cmd, camera_control_req.value);
	ResponseOK(req);
}

static void ChangeSwitchCommandHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeSwitchCommandReq>(req, root))
		return;
	ChangeSwitchCommandReq change_switch_command_req;
	change_switch_command_req = root;
	CONFIG->switch_cmd_ = change_switch_command_req.commands;
	CONFIG->WriteToFile();
	ResponseOK(req);
}

static void ChangeOsdTsHanlder(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeOsdTsReq>(req, root))
		return;
	ChangeOsdTsReq change_osd_ts_req;
	change_osd_ts_req = root;
	CONFIG->osd_ts_ = change_osd_ts_req.osd_ts;
	CONFIG->WriteToFile();
	g_AVManager->CloseOsdTs();
	g_AVManager->StartOsdTs();
	ResponseOK(req);
}

static void ChangeOsdHandler(evhttp_request *req, void *arg)
{
	Json::Value root;
	if (!CheckReq<ChangeOsdReq>(req, root))
		return;
	ChangeOsdReq change_osd_req;
	change_osd_req = root;
	CONFIG->osd_ = change_osd_req.osd;
	CONFIG->WriteToFile();
	g_AVManager->CloseOsd();
	g_AVManager->StartOsd(CONFIG->osd_);
	ResponseOK(req);
}

int32_t main(int32_t argc, char **argv)
{
	Daemon();
	CPUBind::SetCPU(1);
	RTC::LoadTime();

	signal(SIGINT, SignalHandler);
	signal(SIGPIPE, SignalHandler);
	signal(SIGTERM, SignalHandler);

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
			if (CONFIG->Initialize(optarg) != KSuccess)
				return KParamsError;
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
			printf("unknow argument:%c\n", opt);
			return 0;
		}
		}
	}

	if (!got_config_file)
	{
		printf("Usage:%s -c [conf_file_path] -i <http_listen_ip> -p <http_listen_port>\n", argv[0]);
		return 0;
	}

	ConfigLogger();
	MPPSystem::Instance()->Initialize();
	HttpClient::Instance()->Initialize();

	g_AVManager->Initialize();

	g_SerialManager->Initialize(g_Base);
	g_SerialManager->SetEventListener(g_AVManager);
	g_HttpServer->Initialize(ip, port, g_Base);
	g_HttpServer->RegisterURI("/start_local_live", StartLocalLiveHandler, nullptr);
	g_HttpServer->RegisterURI("/stop_local_live", StopLocalLiveHandler, nullptr);
	g_HttpServer->RegisterURI("/start_remote_live", StartRemoteLiveHandler, nullptr);
	g_HttpServer->RegisterURI("/stop_remote_live", StopRemoteLiveHandler, nullptr);
	g_HttpServer->RegisterURI("/start_record", StartRecordHandler, nullptr);
	g_HttpServer->RegisterURI("/stop_record", StopRecordHandler, nullptr);
	g_HttpServer->RegisterURI("/switch", SwitchHandler, nullptr);
	g_HttpServer->RegisterURI("/change_main_screen", ChangeMainScreenHandler, nullptr);
	g_HttpServer->RegisterURI("/change_pc_capture_mode", ChangePCCaptureModeHandler, nullptr);
	g_HttpServer->RegisterURI("/change_display_screen", ChangeDisplayScreenHandler, nullptr);
	g_HttpServer->RegisterURI("/change_video", ChangeVideoHandler, nullptr);
	g_HttpServer->RegisterURI("/save_time", SaveTimeHandler, nullptr);
	g_HttpServer->RegisterURI("/shutdown", ShutDownHandler, nullptr);
	g_HttpServer->RegisterURI("/reboot", ReBootHandler, nullptr);
	g_HttpServer->RegisterURI("/camera_control", CameraControlHandler, nullptr);
	g_HttpServer->RegisterURI("/change_switch_command", ChangeSwitchCommandHandler, nullptr);
	g_HttpServer->RegisterURI("/change_record_mode", ChangeRecordModeHandler, nullptr);
	g_HttpServer->RegisterURI("/change_osd", ChangeOsdHandler, nullptr);
	g_HttpServer->RegisterURI("/change_osd_ts", ChangeOsdTsHanlder, nullptr);

	event_base_dispatch(g_Base);

	g_SerialManager->Close();
	g_HttpServer->Close();
	event_base_free(g_Base);

	g_AVManager->Close(g_Opt);
	HttpClient::Instance()->Close();
	MPPSystem::Instance()->Close();

	if (g_Opt == "shutdown")
	{
		system("poweroff");
	}
	else if (g_Opt == "reboot")
	{
		system("reboot");
	}
	return 0;
}

#include "common/http_client.h"
#include "common/err_code.h"
#include "common/config.h"

namespace rs
{

template <typename T>
void HttpRequestCallBack(struct evhttp_request *req, void *arg)
{
    T *t = static_cast<T *>(arg);
    t->HandleRequestCallBack(req);
}

void HttpClient::HandleRequestCallBack(struct evhttp_request *req)
{
    if (!req)
    {
        log_e("http request failed,%s", strerror(errno));
    }
    else
    {
        switch (req->response_code)
        {
        case HTTP_OK:
            log_d("http request ok");
            break;
        default:
            log_e("http reponse code:%d", req->response_code);
        }
    }
}

HttpClient::HttpClient() : base_(nullptr),
                           init_(false)
{
}

HttpClient::~HttpClient()
{
    Close();
}

void HttpClient::Post(const std::string &data)
{
    Post(CONFIG->web_.ip, CONFIG->web_.port, CONFIG->web_.path, data);
}

void HttpClient::Post(const std::string ip, int port, const std::string &path, const std::string &data)
{
    if (!init_)
        return;

    int ret;

    std::unique_lock<std::mutex> lock(mux_);

    log_d("[post]ip:%s,port:%d,path:%s,data:%s", ip.c_str(), port, path.c_str(), data.c_str());

    evhttp_connection *conn = evhttp_connection_base_new(base_, nullptr, ip.c_str(), port);
    if (!conn)
    {
        log_e("evhttp_connection_base_new failed");
        return;
    }

    evhttp_connection_set_retries(conn, 1);
    evhttp_connection_set_timeout(conn, 1);

    evhttp_request *req = evhttp_request_new(HttpRequestCallBack<HttpClient>, static_cast<void *>(this));
    if (!req)
    {
        log_e("evhttp_request_new failed");
        return;
    }

    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Content-Type", "text/json;charset=UTF-8");
    evhttp_add_header(output_headers, "Host", ip.c_str());
    evhttp_add_header(output_headers, "Connection", "close");

    ret = evbuffer_add(req->output_buffer, data.c_str(), data.length());
    if (ret != KSuccess)
    {
        log_e("evbuffer_add failed");
        return;
    }

    ret = evhttp_make_request(conn, req, EVHTTP_REQ_POST, path.c_str());
    if (ret != KSuccess)
    {
        log_e("evhttp_make_request failed");
        return;
    }

    event_base_dispatch(base_);
    evhttp_connection_free(conn);
}

int HttpClient::Initialize()
{
    if (init_)
        return KInitialized;

    log_d("HTTP_CLIENT start");

    base_ = event_base_new();
    init_ = true;
    return KSuccess;
}

void HttpClient::Close()
{
    if (!init_)
        return;
    log_d("HTTP_CLIENT stop");

    event_base_free(base_);
    init_ = false;
}

HttpClient *HttpClient::Instance()
{
    static HttpClient *instance = new HttpClient();
    return instance;
}

} // namespace rs
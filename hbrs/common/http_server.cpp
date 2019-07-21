#include "common/http_server.h"
#include "common/err_code.h"

namespace rs
{
HttpServer::HttpServer() : base_(nullptr),
                           http_(nullptr),
                           init_(false)
{
}

HttpServer::~HttpServer()
{
    Close();
}

void GenericRequestHandler(evhttp_request *req, void *arg)
{
    evhttp_add_header(req->output_headers, "Server", "hibao record system");
    evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    struct evbuffer *evbuf = evbuffer_new();
    if (!evbuf)
    {
        printf("create evbuffer failed!\n");
        return;
    }
    evbuffer_add_printf(evbuf, "{\"errMsg\":\"unknow uri\"}");
    evhttp_send_reply(req, HTTP_NOTFOUND, "could not find content for uri", evbuf);
    evbuffer_free(evbuf);
}

int HttpServer::Initialize(const std::string &ip, int port)
{
    if (init_)
        return KInitialized;
    int ret;

    base_ = event_base_new();
    http_ = evhttp_new(base_);

    ret = evhttp_bind_socket(http_, ip.c_str(), port);
    if (ret != KSuccess)
    {
        log_e("evhttp_bind_socket failed,%s", strerror(errno));
        return KSystemError;
    }

    evhttp_set_gencb(http_, &GenericRequestHandler, NULL);
    init_ = true;
    return KSuccess;
}

void HttpServer::Close()
{
    if (!init_)
        return;

    evhttp_free(http_);
    event_base_free(base_);

    init_ = false;
}

void HttpServer::RegisterURI(const std::string &uri, RequestHandler *handler, void *arg)
{
    if (!init_)
        return;

    evhttp_set_cb(http_, uri.c_str(), handler, arg);

    return;
}

void HttpServer::Dispatch()
{
    if (!init_)
        return;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; //500ms
    event_base_loopexit(base_, &tv);
    event_base_dispatch(base_);
}
} // namespace rs
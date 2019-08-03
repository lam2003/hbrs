#pragma once

#include "global.h"

namespace rs
{

typedef void RequestHandler(struct evhttp_request *, void *);

class HttpServer
{
public:
    explicit HttpServer();

    virtual ~HttpServer();

    int Initialize(const std::string &ip, int port);

    void RegisterURI(const std::string &uri, RequestHandler *handler, void *arg);

    void Close();

    void Dispatch();

    static void MakeResponse(evhttp_request *req, int code, const std::string &reason, const std::string &str);

    static std::string GetRequestData(evhttp_request *req);

private:
    event_base *base_;
    evhttp *http_;
    bool init_;
};
} // namespace rs
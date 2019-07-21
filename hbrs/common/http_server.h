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

    void RegisterURI(const std::string &uri, RequestHandler *handler,void *arg);

    void Close();

    void Dispatch();

private:
    event_base *base_;
    evhttp *http_;
    bool init_;
};
} // namespace rs
#pragma once

#include "global.h"

namespace rs
{
class HttpClient
{
public:
    virtual ~HttpClient();

    int Initialize();

    void Close();

    void HandleRequestCallBack(struct evhttp_request *req);

    void Post(const std::string ip, int port, const std::string &path, const std::string &data);

    void Post(const std::string &data);

    static HttpClient *Instance();

protected:
    explicit HttpClient();

private:
    std::mutex mux_;
    event_base *base_;
    bool init_;
};
} // namespace rs

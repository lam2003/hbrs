// #pragma once

// #include "global.h"

// void http_requset_post_cb(struct evhttp_request *req, void *arg)
// {
// }

// namespace rs
// {
// class HttpClient
// {
// public:
//     void *http_request_new(struct event_base *base, const std::string &ip, const std::string &path, int port, const std::string data)
//     {
//         struct evhttp_connection *conn = evhttp_connection_base_new(base, NULL, ip.c_str(), port);
//         struct evhttp_request *req = evhttp_request_new(http_requset_post_cb, nullptr);
//         evhttp_make_request(conn, req, EVHTTP_REQ_POST, path.c_str());
//         evbuffer_add(req->output_buffer, data.c_str(), data.length());
//         evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=utf-8");
//         evhttp_add_header(req->output_headers, "Host", ip.c_str());
//     }
// };
// } // namespace rs

#pragma once
#include <evhtp.h>
#include <string>

inline void write_error(evhtp_request_t *req, int error_code, std::string error_msg) {
    evbuffer_add_printf(req->buffer_out, "{\"errorCode\":%d,\"errorDescription\":\"%s\"}",
                        error_code, error_msg.c_str());
    evhtp_send_reply(req, EVHTP_RES_BADREQ);
}

inline std::string get_ip(evhtp_request_t *req) {
    struct sockaddr_in *sin = (struct sockaddr_in *)req->conn->saddr;
    char tmp[1024];
    evutil_inet_ntop(AF_INET, &sin->sin_addr, tmp, sizeof(tmp));
    std::string ip(tmp);
    return ip;
}
    

#pragma once
#include <evhtp.h>
#include <string>

inline void write_error(evhtp_request_t *req, int error_code, std::string error_msg) {
    evbuffer_add_printf(req->buffer_out, "{\"errorCode\":%d,\"errorDescription\":\"%s\"}",
                        error_code, error_msg.c_str());
    evhtp_send_reply(req, EVHTP_RES_BADREQ);
}
    

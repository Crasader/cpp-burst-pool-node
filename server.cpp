#include <regex>
#include <iostream>
#include <string.h>
#include "server.hpp"
#include "Wallet.hpp"
#include "Config.hpp"
#include "DeadlineRequestHandler.hpp"
#include "RateLimiter.hpp"

Wallet *wallet;
Config *cfg;
RateLimiter *rate_limiter;
DeadlineRequestHandler *deadline_request_handler;

void process_submit_nonce_req(evhtp_request_t *req) {
    uint64_t account_id, nonce, height;
    evhtp_query_t *query = req->uri->query;

    const char *account_id_str = evhtp_kv_find(query, "accountId");
    if (account_id_str == nullptr) {
        write_error(req, 1013, "sumitNonce request has bad 'accountId' parameter");
        return;
    }
    try {
        account_id = std::stoull(account_id_str);
    } catch (const std::exception &e) {
        write_error(req, 1013, "sumitNonce request has bad 'accountId' parameter");
        return;
    }
    
    const char *nonce_str = evhtp_kv_find(query, "nonce");
    if (nonce_str == nullptr) {
        write_error(req, 1012, "sumitNonce request has bad 'nonce' parameter");
        return;
    }
    try {
        nonce = std::stoull(nonce_str);
    } catch (const std::exception &e) {
        write_error(req, 1012, "sumitNonce request has bad 'nonce' parameter");
        return;
    }

    Block current_block;
    wallet->get_current_block(current_block);
    const char *height_str = evhtp_kv_find(query, "blockheight");
    if (height_str == nullptr) {
        goto SKIP_BLOCK_CONFIRMATION;
    }
    try {
        height = std::stoull(height_str);
    } catch (const std::exception &e) {
        goto SKIP_BLOCK_CONFIRMATION;
    }
    if (height != current_block._height) {
        write_error(req, 1005, "Submitted on wrong height");
        return;
    }

 SKIP_BLOCK_CONFIRMATION:

    if (!wallet->correct_reward_recipient(account_id)) {
        write_error(req, 1004, "Account's reward recipient doesn't match the pool's");
        return;
    }

    // calculate deadline and answer with worker threads
    std::shared_ptr<CalcDeadlineReq> calc_deadline_req(new CalcDeadlineReq);
    calc_deadline_req->account_id = account_id;
    calc_deadline_req->nonce = nonce;
    calc_deadline_req->scoop_nr = current_block._scoop;
    calc_deadline_req->base_target = current_block._base_target;
    calc_deadline_req->gensig = current_block._gensig;
    calc_deadline_req->height = current_block._height;
    calc_deadline_req->req = req;

    deadline_request_handler->enque_req(calc_deadline_req);

    evhtp_request_pause(req);
}

void process_get_mining_info_request(evhtp_request_t *req) {
    std::string mining_info = wallet->get_cached_mining_info();
    evbuffer_add_printf(req->buffer_out, mining_info.c_str());
    evhtp_send_reply(req, EVHTP_RES_OK);
}

bool limited(evhtp_request_t * req,  const char *request_type) {
    std::string limiter_key = get_ip(req);
    limiter_key.append(request_type);
    if (!rate_limiter->aquire(limiter_key)) {
        evhtp_send_reply(req, 429);
        return true;
    }
    return false;
}

void process_req(evhtp_request_t *req, void *a) {
    evhtp_query_t *query = req->uri->query;

    if (query == nullptr) {
        evhtp_send_reply(req, EVHTP_RES_BADREQ);
        return;
    }

    const char *request_type = evhtp_kv_find(query, "requestType");
    if (request_type == nullptr) {
        evhtp_send_reply(req, EVHTP_RES_BADREQ);
        return;
    }

    if (strcmp(request_type, "getMiningInfo") == 0 ) {
        if (limited(req, request_type)) return;
        process_get_mining_info_request(req);
        return;
    } else if (strcmp(request_type, "submitNonce") == 0 ) {
        if (limited(req, request_type)) return;
        process_submit_nonce_req(req);
        return;
    }

    evhtp_send_reply(req, EVHTP_RES_BADREQ);
}

int main(int argc, char ** argv) {
    evbase_t *evbase;
    evhtp_t *evhtp;

    cfg = new Config("config.json");
    wallet = new Wallet("http://176.9.47.157:6876/burst?requestType=getMiningInfo",
                        "localhost:3306", cfg->_db_name, cfg->_db_user, cfg->_db_password);

    rate_limiter = new RateLimiter(cfg->_allow_requests_per_second, cfg->_burst_size);

    deadline_request_handler = new DeadlineRequestHandler(cfg, wallet);

    evbase = event_base_new();
    evhtp = evhtp_new(evbase, NULL);

    evhtp_set_cb(evhtp, "/burst", process_req, nullptr);
    evhtp_use_threads(evhtp, nullptr, cfg->_connection_thread_count, nullptr);
    evhtp_bind_socket(evhtp, "127.0.0.1", 8124, 1024);

    event_base_loop(evbase, 0);

    return 0;
}


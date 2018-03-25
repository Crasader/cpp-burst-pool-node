#include <stdio.h>
#include <evhtp.h>
#include <string>
#include <regex>
#include <iostream>
#include <string.h>
#include "Wallet.hpp"
#include "Config.hpp"
#include "NodeComClient.hpp"
#include "DeadlineRequestHandler.hpp"

#include <atomic>

std::atomic<int> req_count;

Wallet *wallet;
Config *cfg;
NodeComClient *node_com_client;
DeadlineRequestHandler deadline_request_handler;

void write_error(evhtp_request_t *req, int error_code, std::string error_msg) {
    evbuffer_add_printf(req->buffer_out, "{\"errorCode\":%d,\"errorDescription\":\"%s\"}",
                        error_code, error_msg.c_str());
    evhtp_send_reply(req, EVHTP_RES_BADREQ);
};



void process_submit_nonce_req(evhtp_request_t *req) {
    uint64_t account_id, nonce, height;
    evhtp_query_t *query = req->uri->query;

    req_count++;

    // std::cout << req_count << std::endl;

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

    // uint64_t deadline = deadline_request_handler.calculate_deadline(
    //                                                                 account_id, nonce, current_block._base_target, current_block._scoop,
    //                                                                 (uint8_t *) &current_block._gensig[0]);

    std::cout << current_block._gensig[0] << std::endl;
    deadline_request_handler.calculate_deadline(account_id, nonce, current_block._base_target, current_block._scoop,
                                                (uint8_t *) &current_block._gensig[0], req);

    // if (deadline > cfg->_deadline_limit) {
    //     write_error(req, 1008, "Deadline exceeds deadline limit of the pool");
    //     return;
    // }

    // auto miner_round = wallet->get_miner_round(account_id);

    // if (miner_round != nullptr && miner_round->mu.try_lock()) {
    //     if (miner_round->height == current_block._height && miner_round->deadline > deadline) {
    //         nodecom::SubmitNonceRequest req;
    //         req.set_accountid(account_id);
    //         req.set_nonce(nonce);
    //         req.set_deadline(deadline);
    //         req.set_blockheight(current_block._height);
    //         node_com_client->SubmitNonce(req);
    //         miner_round->deadline = deadline;
    //     }
    //     miner_round->mu.unlock();
    // }

    // evbuffer_add_printf(req->buffer_out, "{\"result\":\"success\",\"deadline\":%lu}", deadline);
    // evhtp_send_reply(req, EVHTP_RES_OK);
}

void process_get_mining_info_request(evhtp_request_t *req) {
    std::string mining_info = wallet->get_cached_mining_info();
    evbuffer_add_printf(req->buffer_out, mining_info.c_str());
    evhtp_send_reply(req, EVHTP_RES_OK);
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
        process_get_mining_info_request(req);
        return;
    } else if (strcmp(request_type, "submitNonce") == 0 ) {
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
    node_com_client = new NodeComClient(grpc::CreateChannel(cfg->_pool_address,
                                                            grpc::InsecureChannelCredentials()));

    evbase = event_base_new();
    evhtp = evhtp_new(evbase, NULL);

    evhtp_set_cb(evhtp, "/burst", process_req, nullptr);
    evhtp_use_threads(evhtp, nullptr, 2, nullptr);
    evhtp_bind_socket(evhtp, "127.0.0.1", 8124, 1024);

    event_base_loop(evbase, 0);

    return 0;
}


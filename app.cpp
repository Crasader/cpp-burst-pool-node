#include <thread>
#include <mutex>
#include <string>
#include <regex>
#include <iostream>
#include "fcgio.h"
#include <stdint.h>
#include "Wallet.hpp"
#include "DeadlineRequestHandler.hpp"
#include "Config.hpp"
#include "NodeComClient.hpp"

const int thread_count = 70;

int sock;
Wallet *wallet;
Config *cfg;
NodeComClient *node_com_client;
DeadlineRequestHandler deadline_request_handler;

std::map<std::string, std::string> params_from_uri(const std::string &query) {
    std::map<std::string, std::string> params;
    std::regex pattern("([\\w+%]+)=([^&]*)");
    auto words_begin = std::sregex_iterator(query.begin(), query.end(), pattern);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; i++) {
        std::string key = (*i)[1].str();
        std::string value = (*i)[2].str();
        params[key] = value;
    }

    return params;
}

void write_submission_success(FCGX_Stream *out, uint64_t deadline) {
    FCGX_FPrintF(out, "{\"result\":\"success\",\"deadline\":%lu}", deadline);
}

void write_error(FCGX_Stream *out, int error_code, std::string error_msg) {
    FCGX_FPrintF(out, "{\"errorCode\":%d,\"errorDescription\":\"%s\"}", error_code, error_msg.c_str());
};

void handle_req(FCGX_Request *req) {
    std::string uri(FCGX_GetParam("REQUEST_URI", req->envp));

    auto params = params_from_uri(uri);

    FCGX_FPrintF(req->out, "Content-type: application/json\r\n\r\n");

    if (params["requestType"] == "getMiningInfo") {
        std::string mining_info = wallet->get_cached_mining_info();
        FCGX_FPrintF(req->out, mining_info.c_str());
    } else if (params["requestType"] == "submitNonce") {
        uint64_t account_id, nonce, height;

        try {
            account_id = std::stoull(params["accountId"]);
        } catch (const std::exception &e) {
            write_error(req->out, 1013, "sumitNonce request has bad 'accountId' parameter");
            return;
        }

        try {
            nonce = std::stoull(params["nonce"]);
        } catch (const std::exception &e) {
            write_error(req->out, 1012, "sumitNonce request has bad 'nonce' parameter");
            return;
        }

        Block current_block;
        wallet->get_current_block(current_block);
        try {
            height = std::stoull(params["blockheight"]);
        } catch (const std::exception &e) {
            // we don't know on which heigh the miner submitted
            goto SKIP_BLOCK_CONFIRMATION;
        }

        if (height != current_block._height) {
            write_error(req->out, 1005, "Submitted on wrong height");
            return;
        }

    SKIP_BLOCK_CONFIRMATION:

        if (!wallet->correct_reward_recipient(account_id)) {
            write_error(req->out, 1004, "Account's reward recipient doesn't match the pool's");
            return;
        }

        uint64_t deadline = deadline_request_handler.calculate_deadline(
            account_id, nonce, current_block._base_target, current_block._scoop,
           (uint8_t *) &current_block._gensig[0]);

        if (deadline > cfg->_deadline_limit) {
            write_error(req->out, 1008, "Deadline exceeds deadline limit of the pool");
            return;
        }

        auto miner_round = wallet->get_miner_round(account_id);

        if (miner_round->mu.try_lock()) {
            if (miner_round->height == current_block._height && miner_round->deadline > deadline) {
                nodecom::SubmitNonceRequest req;
                req.set_accountid(account_id);
                req.set_nonce(nonce);
                req.set_deadline(deadline);
                req.set_blockheight(current_block._height);
                node_com_client->SubmitNonce(req);
                miner_round->deadline = deadline;
            }
            miner_round->mu.unlock();
        }

        write_submission_success(req->out, deadline);
    }
}

void accept_reqs() {
    FCGX_Request req;
    int rc;
    char *server_name;

    FCGX_InitRequest(&req, sock, 0);

    for (;;) {
        static std::mutex accept_mu;

        accept_mu.lock();
        rc = FCGX_Accept_r(&req);
        accept_mu.unlock();

        server_name = FCGX_GetParam("SERVER_NAME", req.envp);

        // TODO: probably better sleep? what happens if nginx restarts?
        if (rc < 0)
            break;

        handle_req(&req);
        FCGX_Finish_r(&req);
    }
}

int main() {
    FCGX_Init();
    sock = FCGX_OpenSocket(":8000", 0);

    cfg = new Config("config.json");
    wallet = new Wallet("http://176.9.47.157:6876/burst?requestType=getMiningInfo",
                        "localhost:3306", cfg->_db_name, cfg->_db_user, cfg->_db_password);
    node_com_client = new NodeComClient(grpc::CreateChannel(cfg->_pool_address,
                                                            grpc::InsecureChannelCredentials()));

    for (int i = 0; i < thread_count; i++) {
        std::thread t(accept_reqs);
        t.join();
    }
}

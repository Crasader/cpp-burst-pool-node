#pragma once 

#include <evhtp.h>
#include <memory>
#include <stdint.h>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <array>
#include "concurrentqueue.h"
#include "Wallet.hpp"
#include "Config.hpp"
#include "NodeComClient.hpp"

struct CalcDeadlineReq {
    uint64_t account_id;
    uint64_t nonce;
    uint64_t height;
    uint32_t scoop_nr;
    uint64_t base_target;
    std::array<uint8_t, 32> gensig;
    evhtp_request_t *req;
};

class DeadlineRequestHandler {
private:
    void distribute_deadline_reqs();

    moodycamel::ConcurrentQueue<std::shared_ptr<CalcDeadlineReq> > _q;
    boost::asio::io_service _io_service;
    boost::asio::io_service::work _work;
    boost::thread_group _threadpool;

    boost::thread *_distribute_thread;

    Wallet *_wallet;
    Config *_cfg;
    NodeComClient *_node_com_client;

    void validate_deadline(std::shared_ptr<CalcDeadlineReq> creq, uint64_t deadline);
    void calculate_deadlines(std::array<std::shared_ptr<CalcDeadlineReq>, 8> creqs, int pending);
public:
    DeadlineRequestHandler(Config *cfg, Wallet *wallet)
        : _cfg(cfg),
          _wallet(wallet),
          _work(_io_service) {
        _node_com_client = new NodeComClient(grpc::CreateChannel(cfg->_pool_address,
                                                                 grpc::InsecureChannelCredentials()));

        for (int i = 0; i < cfg->_validator_thread_count; i++)
            _threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &_io_service));

        _distribute_thread = new boost::thread(boost::bind(&DeadlineRequestHandler::distribute_deadline_reqs, this));
        _distribute_thread->detach();
    };

    ~DeadlineRequestHandler() {
        _distribute_thread->interrupt();
    }

    void enque_req(std::shared_ptr<CalcDeadlineReq> calc_deadline_req);
};

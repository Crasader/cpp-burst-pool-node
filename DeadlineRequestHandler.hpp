#pragma once 

#include <condition_variable>
#include <memory>
#include <stdint.h>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <mutex>
#include "concurrentqueue.h"

struct CalcDeadlineReq {
    uint64_t account_id;
    uint64_t nonce;
    uint64_t deadline;
    bool processed = false;
    std::condition_variable cv;
    std::mutex mu;
};

class DeadlineRequestHandler {
private:
    void distribute_deadline_reqs();

    moodycamel::ConcurrentQueue<CalcDeadlineReq *> _q;
    boost::asio::io_service _io_service;
    boost::asio::io_service::work _work;
    boost::thread_group _threadpool;

    boost::thread* _distribute_thread;

    void add_req(CalcDeadlineReq *req);
public:
    DeadlineRequestHandler(int validator_threads = boost::thread::hardware_concurrency())
        : _work(_io_service) {
        for (int i = 0; i < validator_threads; i++)
            _threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &_io_service));

        _distribute_thread = new boost::thread(boost::bind(&DeadlineRequestHandler::distribute_deadline_reqs, this));
        _distribute_thread->detach();
    };

    ~DeadlineRequestHandler() {
        _distribute_thread->interrupt();
    }

    uint64_t calculate_deadline(uint64_t account_id, uint64_t nonce);
};

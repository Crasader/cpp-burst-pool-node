#pragma once 

#include <memory>
#include <stdint.h>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <array>
#include "Session.hpp"
#include "concurrentqueue.h"
#include "Wallet.hpp"
#include "Config.hpp"
#include "NodeComClient.hpp"

class Session;

struct CalcDeadlineReq {
  uint64_t account_id;
  uint64_t nonce;
  uint64_t height;
  uint32_t scoop_nr;
  uint64_t base_target;
  std::array<uint8_t, 32> gensig;
  std::shared_ptr<Session> session;
};

class DeadlineRequestHandler {
 private:
  void distribute_deadline_reqs();

  moodycamel::ConcurrentQueue<std::shared_ptr<CalcDeadlineReq> > q_;
  boost::asio::io_service io_service_;
  boost::asio::io_service::work work_;
  boost::thread_group threadpool_;

  boost::thread *distribute_thread_;

  Wallet *wallet_;
  NodeComClient *node_com_client_;

  uint64_t deadline_limit_;

  void validate_deadline(std::shared_ptr<CalcDeadlineReq> creq, uint64_t deadline);
  void calculate_deadlines(std::array<std::shared_ptr<CalcDeadlineReq>, 8> creqs, int pending);
 public:
  DeadlineRequestHandler(Config *cfg, Wallet *wallet)
      : deadline_limit_(cfg->deadline_limit_),
        wallet_(wallet),
        work_(io_service_) {
    node_com_client_ = new NodeComClient(cfg->pool_address_);

    for (int i = 0; i < cfg->validator_thread_count_; i++)
      threadpool_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));

    distribute_thread_ = new boost::thread(boost::bind(&DeadlineRequestHandler::distribute_deadline_reqs, this));
    distribute_thread_->detach();
  };

  ~DeadlineRequestHandler() {
    distribute_thread_->interrupt();
  }

  void enque_req(std::shared_ptr<CalcDeadlineReq> calc_deadline_req);
};

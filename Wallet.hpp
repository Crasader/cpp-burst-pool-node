#pragma once 

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <stdint.h>
#include <atomic>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>

class Block {
 public:
  Block(const uint64_t height, const uint64_t base_target):
      height_(height),
      base_target_(base_target) {}
  Block(Block &other);
  Block():
      height_(0),
      base_target_(0) {};
  uint64_t height_, base_target_;
  uint32_t scoop_;
  bool processed = false;
  std::array<uint8_t, 32> gensig_;
};

typedef struct MinerRound {
  std::mutex mu;
  int64_t deadline;
  int64_t height;
} MinerRound;

class Wallet {
 private:
  boost::thread* refresh_blocks_thread_;

  Block current_block_;

  std::string get_mining_info();
  void refresh_blocks();
  bool refresh_block();

  std::atomic<int> cache_idx_;
  std::string mining_info_0_;
  std::string mining_info_1_;

  sql::Driver* driver_;
  sql::Connection* con_;
  sql::Statement* reward_recip_stmt_;

  boost::shared_mutex new_block_mu_;
  std::unordered_map<uint64_t, std::shared_ptr<MinerRound>> miners_;

  const std::string mining_info_uri_;
  const uint64_t deadline_limit_;

  void cache_miners(uint64_t height);
 public:
  Wallet(const std::string wallet_url, std::string db_addr, std::string db_name,
         std::string db_user, std::string db_pw, uint64_t deadline_limit):
      mining_info_uri_(wallet_url + "/burst?requestType=getMiningInfo"),
      deadline_limit_(deadline_limit),
      driver_(get_driver_instance()),
      con_(driver_->connect(db_addr, db_user, db_pw)) {
    con_->setSchema(db_name);
    reward_recip_stmt_ = con_->createStatement();

    bool refreshed = refresh_block();
    assert(refreshed);

    cache_miners(current_block_.height_);

    refresh_blocks_thread_ = new boost::thread(boost::bind(&Wallet::refresh_blocks, this));
    refresh_blocks_thread_->detach();
  }

  ~Wallet() {
    refresh_blocks_thread_->interrupt();
  }
        
  void get_current_block(Block &block);
  std::string get_cached_mining_info();
  std::shared_ptr<MinerRound> get_miner_round(uint64_t account_id);
  bool correct_reward_recipient(uint64_t account_id);
};

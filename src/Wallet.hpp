#pragma once 

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <stdint.h>
#include <atomic>
#include <chrono>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <curl/curl.h>
#include <glog/logging.h>
#include "Config.hpp"

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
  uint64_t deadline;
  uint64_t height;
  uint64_t recip_id;
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

  std::string recip_query_;

  boost::shared_mutex new_block_mu_;
  std::unordered_map<uint64_t, std::shared_ptr<MinerRound>> miners_;

  CURL *curl_;
  std::string mining_info_res_;

  const std::string mining_info_uri_;
  const uint64_t deadline_limit_;

  const std::map<uint64_t, std::string> pool_id_to_addr_;

  void cache_miners(uint64_t height);

  static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
  }

 public:
  Wallet(Config& cfg):
      mining_info_uri_(cfg.wallet_url_ + "/burst?requestType=getMiningInfo"),
      deadline_limit_(cfg.deadline_limit_),
      driver_(get_driver_instance()),
      curl_(curl_easy_init()),
      pool_id_to_addr_(cfg.pool_id_to_addr_) {
    curl_easy_setopt(curl_, CURLOPT_URL, mining_info_uri_.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &mining_info_res_);

    // generate reward recip query string
    recip_query_ = "SELECT CAST(account_id AS UNSIGNED), CAST(recip_id AS UNSIGNED) FROM reward_recip_assign WHERE recip_id IN (" ;
    for (auto it = pool_id_to_addr_.begin(); it != pool_id_to_addr_.end(); it++) {
      uint64_t pool_id = it->first;
      recip_query_ += "CAST(" + std::to_string(pool_id) + " AS SIGNED),";
    }

    // replace trailing ','
    recip_query_[recip_query_.size()-1] = ')';
    recip_query_ += " AND latest = 1";

    int tries = 0;
    for (;;) {
      try {
        con_= driver_->connect(cfg.db_address_, cfg.db_user_, cfg.db_password_);
        break;
      } catch (sql::SQLException& e) {
        LOG(ERROR) << "sql connect: " << e.what();
        tries++;
        if (tries == 10) {
          throw e;
        }
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
      }
    }

    con_->setSchema(cfg.db_name_);
    reward_recip_stmt_ = con_->createStatement();

    refresh_block();

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
  uint64_t get_reward_recipient(uint64_t account_id);
};

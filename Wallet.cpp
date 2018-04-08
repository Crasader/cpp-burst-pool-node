#include <iostream>
#include <chrono>
#include "Wallet.hpp"
#include "burstmath.h"
#include <cppconn/resultset.h>
#include <stdio.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <rapidjson/document.h>
#include <glog/logging.h>

std::string Wallet::get_mining_info() {
  curlpp::Cleanup myCleanup;
  std::ostringstream os;
  try {
    os << curlpp::options::Url(mining_info_uri_);
  } catch (std::exception& e) {
    LOG(ERROR) << "getting mining info: " << e.what();
  }
  return os.str();
}

bool Wallet::refresh_block() {
  uint64_t height, base_target;
  std::string gensig_str;
  rapidjson::Document document;
  std::string mining_info = get_mining_info();

  try {
    document.Parse(mining_info.c_str());

    if (!document.IsObject()) {
      LOG(ERROR) << "parsing mining info: is not an object";
      return false;
    }

    if (!document.HasMember("generationSignature")) {
      LOG(ERROR) << "parsing mining info: missing generation signature";
      return false;
    }

    if (!document.HasMember("height")) {
      LOG(ERROR) << "parsing mining info: missing height";
      return false;
    }

    if (!document.HasMember("baseTarget")) {
      LOG(ERROR) << "parsing mining info: base target";
      return false;
    }

    if (!document["generationSignature"].IsString()) {
      LOG(ERROR) << "parsing mining info: generation signature is not a string";
      return false;
    }

    if (!document["height"].IsString()) {
      LOG(ERROR) << "parsing mining info: height is not a string";
      return false;
    }

    if (!document["baseTarget"].IsString()) {
      LOG(ERROR) << "parsing mining info: base target is not a string";
      return false;
    }

    height = std::stoull(document["height"].GetString());
    base_target = std::stoull(document["baseTarget"].GetString());
    gensig_str = document["generationSignature"].GetString();
  } catch (const std::exception &e) {
    LOG(ERROR) << "parsing mining info: " << e.what();
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
    return false;
  }

  Block current_block;
  get_current_block(current_block);
  if (current_block.height_ < height) {
    boost::upgrade_lock<boost::shared_mutex> lock(new_block_mu_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

    std::array<uint8_t, 32> gensig;
    const char *pos = gensig_str.c_str();
    for (size_t i = 0; i < 32; i++) {
      sscanf( pos, "%2hhx", &gensig[i]);
      pos += 2;
    }

    current_block_.height_ = height;
    current_block_.base_target_ = base_target;
    current_block_.gensig_ = gensig;
    current_block_.scoop_ = calculate_scoop(height, (uint8_t *) &gensig[0]);

    std::string adapted_mining_info = "{\"generationSignature\":\"" + gensig_str +                  + "\"," +
                                      "\"baseTarget\":\""         + std::to_string(base_target)     + "\"," +
                                      "\"height\":\""             + std::to_string(height)          + "\"," +
                                      "\"targetDeadline\":\""     + std::to_string(deadline_limit_) + "\"}";

    int i = cache_idx_.load();
    if (i == 0) {
      mining_info_1_ = adapted_mining_info;
      cache_idx_.store(1);
    } else {
      mining_info_0_ = adapted_mining_info;
      cache_idx_.store(0);
    }
    return true;
  }

  return false;
}

void Wallet::refresh_blocks() {
  for (;;) {
    refresh_block();
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
  }
}

void Wallet::get_current_block(Block &block) {
  boost::upgrade_lock<boost::shared_mutex> lock(new_block_mu_);
  block.height_ = current_block_.height_;
  block.base_target_ = current_block_.base_target_;
  block.gensig_ = current_block_.gensig_;
  block.scoop_ = current_block_.scoop_;
}

std::string Wallet::get_cached_mining_info() {
  int i = cache_idx_.load();

  if (i == 0) {
    return mining_info_0_;
  }
  return mining_info_1_;
}

void Wallet::cache_miners(uint64_t height) {
  sql::ResultSet *res;

  res = reward_recip_stmt_->executeQuery("SELECT CAST(account_id AS UNSIGNED) FROM reward_recip_assign WHERE recip_id = CAST(10282355196851764065 AS SIGNED) AND latest = 1");

  std::unordered_map<uint64_t, std::shared_ptr<MinerRound>> _new_miners;
  while (res->next()) {
    uint64_t account_id = res->getUInt64(1);
    if (miners_.find(account_id) == miners_.end()) {
      std::shared_ptr<MinerRound> miner_round(new MinerRound);
      miner_round->deadline = 0xFFFFFFFFFFFFFFFF;
      miner_round->height = height;
      _new_miners[account_id] = miner_round;
    } else {
      auto miner_round = miners_[account_id];
      miner_round->mu.lock();
      miner_round->deadline = 0xFFFFFFFFFFFFFFFF;
      miner_round->height = height;
      miner_round->mu.unlock();
      _new_miners[account_id] = miner_round;
    }

    miners_ = _new_miners;
  }
  delete res;
}

bool Wallet::correct_reward_recipient(uint64_t account_id) {
  boost::shared_lock<boost::shared_mutex> lock(new_block_mu_);
  bool c = miners_.find(account_id) != miners_.end();
  return c;
}

std::shared_ptr<MinerRound> Wallet::get_miner_round(uint64_t account_id) {
  boost::shared_lock<boost::shared_mutex> lock(new_block_mu_);
  auto miner_round = miners_.find(account_id);
  if (miner_round == miners_.end()) {
    return nullptr;
  } else {
    return miner_round->second;
  }
}

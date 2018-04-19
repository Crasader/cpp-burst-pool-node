#pragma once
#include <rapidjson/document.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <thread>
#include <map>
#include <glog/logging.h>

using namespace rapidjson;

class Config {
 public:
  Config(std::string cfg_file) {
    std::ifstream config_file(cfg_file);
    std::string config_str((std::istreambuf_iterator<char>(config_file)),
                           std::istreambuf_iterator<char>());
    Document document;

    document.Parse(config_str.c_str());
    assert(document.IsObject());

    assert(document.HasMember("deadlineLimit"));
    assert(document["deadlineLimit"].IsUint64());
    deadline_limit_ = document["deadlineLimit"].GetUint64();

    assert(document.HasMember("listenAddress"));
    assert(document["listenAddress"].IsString());
    listen_address_ = document["listenAddress"].GetString();

    assert(document.HasMember("listenPort"));
    assert(document["listenPort"].IsInt());
    listen_port_ = document["listenPort"].GetInt();

    assert(document.HasMember("poolIdToAddress"));
    Value& pool_id_to_addr = document["poolIdToAddress"];
    assert(pool_id_to_addr.IsObject());

    for (Value::ConstMemberIterator it = pool_id_to_addr.MemberBegin(); it != pool_id_to_addr.MemberEnd(); it++) {
      assert(it->name.IsString());
      assert(it->value.IsString());

      std::string pool_id_str = it->name.GetString();
      std::string addr = it->value.GetString();

      uint64_t pool_id;
      try {
        pool_id = std::stoull(pool_id_str);
      } catch (const std::exception& e) {
        LOG(ERROR) << "pool id has wrong format: " << pool_id_str << " should be uint64";
        throw e;
      }

      pool_id_to_addr_[pool_id] = addr;
    }

    assert(document.HasMember("dbAddress"));
    assert(document["dbAddress"].IsString());
    db_address_ = document["dbAddress"].GetString();

    assert(document.HasMember("dbName"));
    assert(document["dbName"].IsString());
    db_name_ = document["dbName"].GetString();

    assert(document.HasMember("dbUser"));
    assert(document["dbUser"].IsString());
    db_user_ = document["dbUser"].GetString();

    assert(document.HasMember("dbPassword"));
    assert(document["dbPassword"].IsString());
    db_password_ = document["dbPassword"].GetString();

    if (document.HasMember("connectionThreadCount")) {
      assert(document["connectionThreadCount"].IsInt());
      connection_thread_count_ = document["connectionThreadCount"].GetInt();
    } else {
      connection_thread_count_ = std::thread::hardware_concurrency();
      LOG(INFO) << "using " << connection_thread_count_ << " as default number of cores for connections";
    }

    if (document.HasMember("validatorThreadCount")) {
      assert(document["validatorThreadCount"].IsInt());
      validator_thread_count_ = document["validatorThreadCount"].GetInt();
    } else {
      validator_thread_count_ = std::thread::hardware_concurrency();
      LOG(INFO) << "using " << validator_thread_count_ << " as default number of cores for deadline validations";
    }

    assert(document.HasMember("allowRequestsPerSecond"));
    assert(document["allowRequestsPerSecond"].IsDouble());
    allow_requests_per_second_ = document["allowRequestsPerSecond"].GetDouble();

    assert(document.HasMember("burstSize"));
    assert(document["burstSize"].IsDouble());
    burst_size_ = document["burstSize"].GetDouble();

    assert(document.HasMember("walletUrl"));
    assert(document["walletUrl"].IsString());
    wallet_url_ = document["walletUrl"].GetString();

    assert(document.HasMember("PoC2StartHeight"));
    assert(document["PoC2StartHeight"].IsUint64());
    poc2_start_height_ = document["PoC2StartHeight"].GetUint64();
  };

  std::map<uint64_t, std::string> pool_id_to_addr_;

  uint64_t deadline_limit_;

  std::string listen_address_;
  uint16_t listen_port_;

  std::string wallet_url_;

  std::string db_address_;
  std::string db_name_;
  std::string db_user_;
  std::string db_password_;

  int validator_thread_count_;
  int connection_thread_count_;

  double allow_requests_per_second_;
  double burst_size_;

  uint64_t poc2_start_height_;
};

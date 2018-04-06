#pragma once
#include <rapidjson/document.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdint.h>

class Config {
 public:
  Config(const std::string &cfg_file) {
    std::ifstream config_file(cfg_file);
    std::string config_str((std::istreambuf_iterator<char>(config_file)),
                           std::istreambuf_iterator<char>());
    rapidjson::Document document;

    document.Parse(config_str.c_str());
    assert(document.IsObject());

    assert(document.HasMember("secret"));
    assert(document["secret"].IsString());
    secret_ = document["secret"].GetString();

    assert(document.HasMember("deadlineLimit"));
    assert(document["deadlineLimit"].IsUint64());
    deadline_limit_ = document["deadlineLimit"].GetUint64();

    assert(document.HasMember("listenAddress"));
    assert(document["listenAddress"].IsString());
    listen_address_ = document["listenAddress"].GetString();

    assert(document.HasMember("listenPort"));
    assert(document["listenPort"].IsInt());
    listen_port_ = document["listenPort"].GetInt();

    assert(document.HasMember("poolPublicId"));
    assert(document["poolPublicId"].IsUint64());
    pool_public_id_ = document["poolPublicId"].GetUint64();

    assert(document.HasMember("poolAddress"));
    assert(document["poolAddress"].IsString());
    pool_address_ = document["poolAddress"].GetString();

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

    assert(document.HasMember("connectionThreadCount"));
    assert(document["connectionThreadCount"].IsInt());
    connection_thread_count_ = document["connectionThreadCount"].GetInt();

    assert(document.HasMember("validatorThreadCount"));
    assert(document["validatorThreadCount"].IsInt());
    validator_thread_count_ = document["validatorThreadCount"].GetInt();

    assert(document.HasMember("allowRequestsPerSecond"));
    assert(document["allowRequestsPerSecond"].IsDouble());
    allow_requests_per_second_ = document["allowRequestsPerSecond"].GetDouble();

    assert(document.HasMember("burstSize"));
    assert(document["burstSize"].IsDouble());
    burst_size_ = document["burstSize"].GetDouble();

    assert(document.HasMember("walletUrl"));
    assert(document["walletUrl"].IsString());
    wallet_url_ = document["walletUrl"].GetString();
  };

  std::string secret_;

  uint64_t deadline_limit_;

  std::string listen_address_;
  uint16_t listen_port_;

  uint64_t pool_public_id_;
  std::string pool_address_;

  std::string wallet_url_;

  std::string db_address_;
  std::string db_name_;
  std::string db_user_;
  std::string db_password_;

  int validator_thread_count_;
  int connection_thread_count_;

  double allow_requests_per_second_;
  double burst_size_;
};

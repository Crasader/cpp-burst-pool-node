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
        _secret = document["secret"].GetString();

        assert(document.HasMember("deadlineLimit"));
        assert(document["deadlineLimit"].IsUint64());
        _deadline_limit = document["deadlineLimit"].GetUint64();

        assert(document.HasMember("listenAddress"));
        assert(document["listenAddress"].IsString());
        _listen_address = document["listenAddress"].GetString();

        assert(document.HasMember("listenPort"));
        assert(document["listenPort"].IsInt());
        _listen_port = document["listenPort"].GetInt();

        assert(document.HasMember("poolPublicId"));
        assert(document["poolPublicId"].IsUint64());
        _pool_public_id = document["poolPublicId"].GetUint64();

        assert(document.HasMember("poolAddress"));
        assert(document["poolAddress"].IsString());
        _pool_address = document["poolAddress"].GetString();

        assert(document.HasMember("dbAddress"));
        assert(document["dbAddress"].IsString());
        _db_address = document["dbAddress"].GetString();

        assert(document.HasMember("dbName"));
        assert(document["dbName"].IsString());
        _db_name = document["dbName"].GetString();

        assert(document.HasMember("dbUser"));
        assert(document["dbUser"].IsString());
        _db_user = document["dbUser"].GetString();

        assert(document.HasMember("dbPassword"));
        assert(document["dbPassword"].IsString());
        _db_password = document["dbPassword"].GetString();
    };

    std::string _secret;

    uint64_t _deadline_limit;

    std::string _listen_address;
    uint16_t _listen_port;

    uint64_t _pool_public_id;
    std::string _pool_address;

    std::string _db_address;
    std::string _db_name;
    std::string _db_user;
    std::string _db_password;
};

#pragma once 

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <stdint.h>
#include <atomic>
#include <Poco/URI.h>
#include <Poco/JSON/Parser.h>
#include "Poco/RWLock.h"
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <array>
#include <unordered_set>

class Block {
public:
    Block(const Poco::UInt64 height, const Poco::UInt64 base_target):
        _height(height),
        _base_target(base_target) {}
    Block(Block &other);
    Block():
        _height(0),
        _base_target(0) {};
    uint64_t _height, _base_target;
    uint32_t _scoop;
    bool processed = false;
    std::array<uint8_t, 32> _gensig;
};

class Wallet {
private:
    boost::thread* _refresh_block_thread;

    Block _current_block;
    Poco::RWLock _current_block_mu;

    std::string get_mining_info();
    void refresh_block();

    std::atomic<int> _cache_idx;
    std::string _mining_info_0;
    std::string _mining_info_1;

    sql::Driver *_driver;
    sql::Connection *_con;
    sql::Statement *_reward_recip_stmt;

    boost::shared_mutex _new_block_mu;
    std::unordered_set<uint64_t> _recipients;

    const std::string _mining_info_uri;

    void cache_reward_recipients();
public:
    Wallet(const std::string mining_info_uri, std::string db_addr, std::string db_name,
           std::string db_user, std::string db_pw):
        _mining_info_uri(mining_info_uri),

        _driver(get_driver_instance()),
        _con(_driver->connect(db_addr, db_user, db_pw)) {
        _con->setSchema(db_name);
        _reward_recip_stmt = _con->createStatement();

        cache_reward_recipients();

        _refresh_block_thread = new boost::thread(boost::bind(&Wallet::refresh_block, this));
        _refresh_block_thread->detach();
    }

    ~Wallet() {
        _refresh_block_thread->interrupt();
    }
        
    void get_current_block(Block &block);
    std::string get_cached_mining_info();
    bool correct_reward_recipient(uint64_t account_id);
};

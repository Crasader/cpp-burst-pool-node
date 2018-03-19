#include <iostream>
#include <chrono>
#include "Wallet.hpp"
#include "burstmath.h"
#include <cppconn/resultset.h>
#include <stdio.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <rapidjson/document.h>

std::string Wallet::get_mining_info() {
    curlpp::Cleanup myCleanup;
    std::ostringstream os;
    os << curlpp::options::Url(_mining_info_uri);
    return os.str();
}

void Wallet::refresh_block() {
    for (;;) {
        uint64_t height, base_target;
        std::string gensig_str;
        rapidjson::Document document;
        std::string mining_info = get_mining_info();

        try {
            document.Parse(mining_info.c_str());
            assert(document.IsObject());

            assert(document.HasMember("generationSignature"));
            assert(document.HasMember("height"));
            assert(document.HasMember("baseTarget"));

            assert(document["generationSignature"].IsString());
            assert(document["height"].IsString());
            assert(document["baseTarget"].IsString());

            height = std::stoull(document["height"].GetString());
            base_target = std::stoull(document["baseTarget"].GetString());
            gensig_str = document["generationSignature"].GetString();
        } catch (const std::exception &e) {
            boost::this_thread::sleep_for(boost::chrono::seconds(1));
            continue;
        }

        Block current_block;
        get_current_block(current_block);
        if (current_block._height < height) {
            boost::upgrade_lock<boost::shared_mutex> lock(_new_block_mu);
            boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

            std::array<uint8_t, 32> gensig;
            const char *pos = gensig_str.c_str();
            for (size_t i = 0; i < 32; i++) {
                sscanf( pos, "%2hhx", &gensig[i]);
                pos += 2;
            }

            _current_block._height = height;
            _current_block._base_target = base_target;
            _current_block._gensig = gensig;
            _current_block._scoop = calculate_scoop(height, (uint8_t *) &gensig[0]);

            int i = _cache_idx.load();
            if (i == 0) {
                _mining_info_1 = mining_info;
                _cache_idx.store(1);
            } else {
                _mining_info_0 = mining_info;
                _cache_idx.store(0);
            }
        }

        boost::this_thread::sleep_for(boost::chrono::seconds(1));
    }
}

void Wallet::get_current_block(Block &block) {
    boost::upgrade_lock<boost::shared_mutex> lock(_new_block_mu);
    block._height = _current_block._height;
    block._base_target = _current_block._base_target;
    block._gensig = _current_block._gensig;
    block._scoop = _current_block._scoop;
}

std::string Wallet::get_cached_mining_info() {
    int i = _cache_idx.load();

    if (i == 0) {
        return _mining_info_0;
    }
    return _mining_info_1;
}

void Wallet::cache_reward_recipients() {
      sql::ResultSet *res;

      res = _reward_recip_stmt->executeQuery("SELECT CAST(account_id AS UNSIGNED) FROM reward_recip_assign WHERE recip_id = CAST(10282355196851764065 AS SIGNED) AND latest = 1");

      _recipients.clear();
      while (res->next()) {
          _recipients.insert(res->getUInt64(1));
      }
      delete res;
}

bool Wallet::correct_reward_recipient(uint64_t account_id) {
    boost::shared_lock<boost::shared_mutex> lock(_new_block_mu);
    int c = _recipients.count(account_id);
    return c > 0;
}

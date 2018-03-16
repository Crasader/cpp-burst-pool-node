#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/Exception.h>
#include <iostream>
#include <chrono>
#include "Wallet.hpp"
#include "burstmath.h"
#include <cppconn/resultset.h>

using namespace Poco::Net;
using namespace Poco;
using namespace std;

string Wallet::get_mining_info() {
    try {
        _client.sendRequest(_mining_info_req);

        HTTPResponse res;
        istream &is = _client.receiveResponse(res);

        string content;
        StreamCopier::copyToString(is, content);
        return content;
    } catch (Exception &ex) {
        cerr << ex.displayText() << endl;
        return "";
    }
}

void Wallet::refresh_block() {
    while (1) {
        Poco::JSON::Parser parser;
        std::string mining_info = get_mining_info();

        Poco::JSON::Object::Ptr root;
        try {
            root = parser.parse(mining_info).extract<Poco::JSON::Object::Ptr>();
        } catch(Exception &ex) {
            cerr << ex.displayText() << endl;
            boost::this_thread::sleep_for(boost::chrono::seconds(1));
            continue;
        }

        uint64_t height, base_target;
        std::string gen_sig;
        if (root->has("height")) {
            height = (uint64_t) root->get("height");
        } else {
            return;
        }

        if (root->has("baseTarget")) {
            base_target = (uint64_t) root->get("baseTarget");
        } else {
            return;
        }

        if (root->has("generationSignature")) {
            gen_sig = root->get("generationSignature").convert<std::string>();
        } else {
            return;
        }

        Block current_block;
        get_current_block(current_block);
        if (current_block._height < height) {
            boost::upgrade_lock<boost::shared_mutex> lock(_new_block_mu);
            boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

            _current_block_mu.writeLock();
            current_block._height = height;
            current_block._base_target = base_target;
            current_block._gen_sig = gen_sig;
            current_block._scoop = calculate_scoop(height, (uint8_t *) gen_sig.c_str());
            _current_block_mu.unlock();

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
    _current_block_mu.readLock();
    block._height = _current_block._height;
    block._base_target = _current_block._base_target;
    block._gen_sig = _current_block._gen_sig;
    block._scoop = _current_block._scoop;
    _current_block_mu.unlock();
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

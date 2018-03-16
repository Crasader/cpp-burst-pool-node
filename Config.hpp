#include <Poco/JSON/Parser.h>
#include <Poco/Exception.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdint.h>

using namespace Poco;
using namespace std;

class Config {
public:
    Config(const std::string &cfg_file) {
        std::ifstream config_file(cfg_file);
        std::string config_str((std::istreambuf_iterator<char>(config_file)),
                               std::istreambuf_iterator<char>());

        Poco::JSON::Parser parser;
        Poco::JSON::Object::Ptr root;

        try {
            root = parser.parse(config_str).extract<Poco::JSON::Object::Ptr>();
        } catch(Exception &ex) {
            cerr << ex.displayText() << endl;
            return;
        }

        if (root->has("secret")) {
            secret = root->get("secret").convert<std::string>();
        } else {
            cerr << "missing 'secret' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("deadlineLimit")) {
            deadline_limit = (uint64_t) root->get("deadlineLimit");
        } else {
            cerr << "missing 'deadline_limit' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("listenAddress")) {
            listen_address = root->get("listenAddress").convert<std::string>();
        } else {
            cerr << "missing 'listenAddress' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("listenPort")) {
            listen_port = (uint16_t) root->get("listenPort");
        } else {
            cerr << "missing 'listenPort' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("poolPublicId")) {
            pool_public_id = (uint64_t) root->get("poolPublicId");
        } else {
            cerr << "missing 'poolPublicId' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("poolAddress")) {
            pool_address = root->get("poolAddress").convert<std::string>();
        } else {
            cerr << "missing 'poolAddress' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("dbAddress")) {
            db_address = root->get("dbAddress").convert<std::string>();
        } else {
            cerr << "missing 'dbAddress' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("dbName")) {
            db_name = root->get("dbName").convert<std::string>();
        } else {
            cerr << "missing 'dbName' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("dbUser")) {
            db_user = root->get("dbUser").convert<std::string>();
        } else {
            cerr << "missing 'dbUser' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        if (root->has("dbPassword")) {
            db_password = root->get("dbPassword").convert<std::string>();
        } else {
            cerr << "missing 'dbPassword' in config" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }
    };

    std::string secret;

    uint64_t deadline_limit;

    std::string listen_address;
    uint16_t listen_port;

    uint64_t pool_public_id;
    std::string pool_address;

    std::string db_address;
    std::string db_name;
    std::string db_user;
    std::string db_password;
};

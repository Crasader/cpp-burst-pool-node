#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/PartHandler.h"
#include "Poco/Net/MessageHeader.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/CountingStream.h"
#include "Poco/NullStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include <iostream>
#include <fstream>
#include <memory>
#include "Wallet.hpp"
#include "DeadlineRequestHandler.hpp"
#include "Config.hpp"
#include "NodeComClient.hpp"

using Poco::Net::ServerSocket;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPServerParams;
using Poco::Net::MessageHeader;
using Poco::Net::HTMLForm;
using Poco::Net::NameValueCollection;
using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::CountingInputStream;
using Poco::NullOutputStream;
using Poco::StreamCopier;

Wallet *wallet;
Config *config;
NodeComClient *node_com_client;

DeadlineRequestHandler deadline_request_handler;

class PartHandler: public Poco::Net::PartHandler {
public:
    PartHandler() {}

    void handlePart(const MessageHeader &header, std::istream &stream) {
        CountingInputStream istr(stream);
        NullOutputStream ostr;
        StreamCopier::copyStream(istr, ostr);
    }
};


class FormRequestHandler: public HTTPRequestHandler {
public:
    FormRequestHandler() {}

    void handleRequest(HTTPServerRequest &request, HTTPServerResponse &response) {
        PartHandler partHandler;
        HTMLForm form(request, request.stream(), partHandler);

        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");

        std::ostream &ostr = response.send();

        if (form["requestType"] == "getMiningInfo") {
            std::string mining_info = wallet->get_cached_mining_info();
            ostr << mining_info;
        } else if (form["requestType"] == "submitNonce") {
            uint64_t account_id, nonce;
            // TODO: check blockheight
            try {
                account_id = std::stoull(form["accountId"]);
            } catch (const std::exception &e) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                writeJSONError(ostr, 1013, "sumitNonce request has bad 'accountId' parameter");
                return;
            }

            try {
                nonce = std::stoull(form["nonce"]);
            } catch (const std::exception &e) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                writeJSONError(ostr, 1012, "sumitNonce request has bad 'nonce' parameter");
                return;
            }

            if (!wallet->correct_reward_recipient(account_id)) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
                writeJSONError(ostr, 1004, "Account's reward recipient doesn't match the pool's");
                return;
            }
            
            uint64_t deadline = deadline_request_handler.calculate_deadline(account_id, nonce);
            if (deadline > config->deadline_limit) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                writeJSONError(ostr, 1008, "Deadline exceeds deadline limit of the pool");
                return;
            }

            nodecom::SubmitNonceRequest req;
            req.set_nonce(nonce);
            req.set_deadline(deadline);
            req.set_accountid(account_id);
            req.set_blockheight(0);
            req.set_secret(config->secret);
            bool success = node_com_client->SubmitNonce(req);
            if (!success) {
                // TODO: log!
            }

            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            ostr << "{result:\"success\",deadline:" << deadline << "}";
        }
    }

    void writeJSONError(std::ostream &ostr, int error_code, std::string error_msg) {
        ostr << "{errorCode:" << error_code << ",errorDescription:\"" << error_msg << "\"}";
    }
};


class FormRequestHandlerFactory: public HTTPRequestHandlerFactory {
public:
    FormRequestHandlerFactory() {}

    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &request) {
        return new FormRequestHandler;
    }
};


class HTTPFormServer: public Poco::Util::ServerApplication {
public:
    HTTPFormServer() {}

    ~HTTPFormServer() {}

protected:
    void initialize(Application &self) {
        ServerApplication::initialize(self);
    }

    void uninitialize() {
        ServerApplication::uninitialize();
    }

    int main(const std::vector<std::string> &args) {
        unsigned short port = 9980;

        ServerSocket svs(port);
	Poco::Net::HTTPServerParams *params = new Poco::Net::HTTPServerParams();
	params->setMaxQueued(2000);
	params->setMaxThreads(100);

        HTTPServer srv(new FormRequestHandlerFactory, svs, params);
        srv.start();
        waitForTerminationRequest();
        srv.stop();

        return Application::EXIT_OK;
    }
};


int main(int argc, char **argv) {
    HTTPFormServer app;

    config = new Config("config.json");
    wallet = new Wallet(Poco::URI("http://176.9.47.157:6876/burst?requestType=getMiningInfo"),
                        "localhost:3306", config->db_name, config->db_user, config->db_password);
    node_com_client = new NodeComClient(grpc::CreateChannel(config->pool_address,
                                                            grpc::InsecureChannelCredentials()));

    return app.run(argc, argv);
}

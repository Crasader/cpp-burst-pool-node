#pragma once 

#include <memory>
#include <string>
#include <iostream>
#include <grpc++/grpc++.h>

#include "nodecom.grpc.pb.h"

class NodeComClient {
public:
    NodeComClient(std::shared_ptr<grpc::Channel> channel)
        : _stub(nodecom::NodeCom::NewStub(channel)) {}

    bool SubmitNonce(const nodecom::SubmitNonceRequest &req) {
        nodecom::SubmitNonceReply res;
        grpc::ClientContext context;
        grpc::Status status = _stub->SubmitNonce(&context, req, &res);
        if (status.ok()) {
            return true;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return false;
        }
    }
private:
    std::unique_ptr<nodecom::NodeCom::Stub> _stub;
};


        

#pragma once 

#include <memory>
#include <string>
#include <iostream>
#include <grpc++/grpc++.h>
#include <glog/logging.h>
#include <chrono>
#include "Config.hpp"
#include "nodecom.grpc.pb.h"

class NodeComClient {
 public:
  NodeComClient(std::string pool_address)
      : channel_(grpc::CreateChannel(pool_address, grpc::InsecureChannelCredentials())),
        stub_(nodecom::NodeCom::NewStub(channel_)) {}

  bool SubmitNonce(const nodecom::SubmitNonceRequest &req) {
    nodecom::SubmitNonceReply res;
    grpc::ClientContext context;
    grpc::Status status = stub_->SubmitNonce(&context, req, &res);
    if (status.ok()) {
      return true;
    } else {
      LOG(ERROR) << status.error_code() << ": " << status.error_message();
      return false;
    }
  }
 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<nodecom::NodeCom::Stub> stub_;
};


        

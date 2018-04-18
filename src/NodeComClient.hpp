#pragma once 

#include <memory>
#include <string>
#include <iostream>
#include <grpc++/grpc++.h>
#include <glog/logging.h>
#include <chrono>
#include <thread>
#include "Config.hpp"
#include "nodecom.grpc.pb.h"

using namespace nodecom;
using grpc::Status;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;

class NodeComClient {
 public:
  NodeComClient(std::string pool_address)
      : channel_(grpc::CreateChannel(pool_address, grpc::InsecureChannelCredentials())),
        stub_(NodeCom::NewStub(channel_)) {

    std::thread thread = std::thread(&NodeComClient::async_complete_rpc, this);
    thread.detach();
  }

  void do_submit_nonce(const SubmitNonceRequest &req) {
    SubmitNonceReply res;
    grpc::ClientContext context;

    AsyncClientCall* call = new AsyncClientCall;

    call->response_reader = stub_->PrepareAsyncSubmitNonce(&call->context, req, &cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }
  void async_complete_rpc() {
    void* got_tag;
    bool ok = false;

    while (cq_.Next(&got_tag, &ok)) {
      AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

      if (!call->status.ok())
        LOG(ERROR) << "rpc failed: " << " " <<  call->status.error_code() << " "<< call->status.error_message();

      delete call;
    }
  }
 private:
  struct AsyncClientCall {
    SubmitNonceReply reply;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<SubmitNonceReply>> response_reader;
  };

  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<NodeCom::Stub> stub_;
  grpc::CompletionQueue cq_;
};


        

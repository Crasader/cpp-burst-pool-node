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
 private:
  struct Server {
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<NodeCom::Stub> stub;
  };

  struct AsyncClientCall {
    SubmitNonceReply reply;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<SubmitNonceReply>> response_reader;
  };

  std::map<uint64_t, Server> pool_id_to_server_;

  grpc::CompletionQueue cq_;

 public:
  NodeComClient(std::map<uint64_t, std::string> pool_id_to_addr) {
    for (auto it = pool_id_to_addr.begin(); it != pool_id_to_addr.end(); it++) {
      auto channel = grpc::CreateChannel(it->second, grpc::InsecureChannelCredentials());
      pool_id_to_server_[it->first].channel = channel;
      pool_id_to_server_[it->first].stub = NodeCom::NewStub(channel);
    }

    std::thread thread = std::thread(&NodeComClient::async_complete_rpc, this);
    thread.detach();
  }

  void do_submit_nonce(uint64_t recip_id, SubmitNonceRequest req) {
    auto search = pool_id_to_server_.find(recip_id);
    if (search == pool_id_to_server_.end()) {
      LOG(ERROR) << "no pool server found that serves recip " << recip_id;
      return;
    }

    AsyncClientCall* call = new AsyncClientCall;
    call->response_reader = search->second.stub->PrepareAsyncSubmitNonce(&call->context, req, &cq_);
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
};

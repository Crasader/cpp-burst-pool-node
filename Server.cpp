#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <memory>
#include <map>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <glog/logging.h>
#include <stdio.h>
#include "Config.hpp"
#include "Wallet.hpp"
#include "Session.hpp"
#include "RateLimiter.hpp"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

Config* cfg;
Wallet* wallet;
DeadlineRequestHandler* deadline_req_handler;
RateLimiter* rate_limiter;

class Listener : public std::enable_shared_from_this<Listener> {
  tcp::acceptor acceptor_;
  tcp::socket socket_;

 public:
  Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint)
      : acceptor_(ioc),
        socket_(ioc) {
    boost::system::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if(ec) {
      LOG(ERROR) << "open: " << ec.message();
      return;
    }

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    if(ec) {
      LOG(ERROR) << "set_option: " << ec.message();
      return;
    }

    acceptor_.bind(endpoint, ec);
    if(ec) {
      LOG(ERROR) << "bind: " << ec.message();
      return;
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if(ec) {
      LOG(ERROR) << "listen: " << ec.message();
      return;
    }
  }

  void run() {
    if(! acceptor_.is_open())
      return;
    do_accept();
  }

  void do_accept() {
    acceptor_.async_accept(socket_, std::bind(
        &Listener::on_accept,
        shared_from_this(),
        std::placeholders::_1));
  }

  void on_accept(boost::system::error_code ec) {
    if(ec) {
      LOG(ERROR) << "accept: " << ec.message();
    } else {
      std::make_shared<Session>(std::move(socket_), wallet, deadline_req_handler, rate_limiter)->run();
    }

    do_accept();
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  cfg = new Config("config.json");
  wallet = new Wallet(cfg->wallet_url_, cfg->db_address_, cfg->db_name_, cfg->db_user_, cfg->db_password_);
  deadline_req_handler = new DeadlineRequestHandler(cfg, wallet);
  rate_limiter = new RateLimiter(cfg->allow_requests_per_second_, cfg->burst_size_);

  auto const address = boost::asio::ip::make_address(cfg->listen_address_);
  auto const port = cfg->listen_port_;
  auto const threads = cfg->connection_thread_count_;

  boost::asio::io_context ioc{threads};

  std::make_shared<Listener>(ioc, tcp::endpoint{address, port})->run();

  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for(auto i = threads - 1; i > 0; --i)
    v.emplace_back([&ioc] { ioc.run(); });
  ioc.run();

  return 0;
}

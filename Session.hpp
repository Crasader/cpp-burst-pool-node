#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include "Wallet.hpp"
#include "DeadlineRequestHandler.hpp"

class DeadlineRequestHandler;

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

class Session : public std::enable_shared_from_this<Session> {
  tcp::socket socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  std::shared_ptr<void> res_;
  Wallet* wallet_;
  DeadlineRequestHandler* deadline_req_handler_;
 public:
  explicit Session(tcp::socket socket, Wallet* wallet, DeadlineRequestHandler* deadline_req_handler)
      : socket_(std::move(socket)),
        strand_(socket_.get_executor()),
        wallet_(wallet),
        deadline_req_handler_(deadline_req_handler) {}

  void run();

  void do_read();

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

  void handle_request();

  void process_submit_nonce_req(std::map<std::string, std::string>& params);

  void writeJSONError(int error_code, std::string error_msg);

  void write_ok(std::string& msg);

  void do_write(http::response<http::string_body>& res);

  void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close);

  void do_close();
};



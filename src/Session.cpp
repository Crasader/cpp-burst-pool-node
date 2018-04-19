#include "Session.hpp"
#include <string>
#include <map>
#include <regex>
#include <glog/logging.h>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

std::map<std::string, std::string> parse_url(const std::string& query) {
  std::map<std::string, std::string> params;
  std::regex pattern("([\\w+%]+)=([^&]*)");
  auto words_begin = std::sregex_iterator(query.begin(), query.end(), pattern);
  auto words_end = std::sregex_iterator();

  for (std::sregex_iterator i = words_begin; i != words_end; i++) {
    std::string key = (*i)[1].str();
    std::string value = (*i)[2].str();
    params[key] = value;
  }

  return params;
}

void Session::run() {
  do_read();
}

void Session::do_read() {
  // Make the request empty before reading,
  // otherwise the operation behavior is undefined.
  req_ = {};

  // Read a request
  http::async_read(socket_, buffer_, req_, boost::asio::bind_executor(
      strand_,
      std::bind(
          &Session::on_read,
          shared_from_this(),
          std::placeholders::_1,
          std::placeholders::_2)));
}

void Session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if(ec == http::error::end_of_stream) {
    do_close();
    return;
  }

  if(ec) {
    LOG(ERROR) << "read: " << ec.message();
    return ;
  }

  handle_request();
}

void Session::handle_request() {
  std::map<std::string, std::string> params = parse_url(req_.target().to_string());

  boost::asio::ip::address addr = socket_.local_endpoint().address();
  std::string limiter_key = addr.to_string();

  auto req_type = params.find("requestType");
  if (req_type == params.end()) {
    http::response<http::string_body> res{http::status::bad_request, req_.version()};
    do_write(res);
    return;
  }

  std::string req_type_str = req_type->second;
  limiter_key += req_type_str;

  if (!rl_->aquire(limiter_key)) {
    http::response<http::string_body> res{http::status::service_unavailable, req_.version()};
    do_write(res);
    LOG(ERROR) << "rate limit exceeded: " << addr;
    return;
  }

  if (req_type_str == "getMiningInfo") {
    limiter_key += "getMiningInfo";
    http::response<http::string_body> res{http::status::ok, req_.version()};
    res.body() = wallet_->get_cached_mining_info();
    do_write(res);
  } else if (req_type_str == "submitNonce") {
    limiter_key += "submitNonce";
    process_submit_nonce_req(params);
  } else {
    http::response<http::string_body> res{http::status::bad_request, req_.version()};
    do_write(res);
  }
}

void Session::process_submit_nonce_req(std::map<std::string, std::string>& params) {
  uint64_t account_id, nonce, recip_id;

  try {
    account_id = std::stoull(params["accountId"]);
  } catch (const std::exception& e) {
    writeJSONError(1013, "sumitNonce request has bad 'accountId' parameter");
    return;
  }

  try {
    nonce = std::stoull(params["nonce"]);
  } catch (const std::exception& e) {
    writeJSONError(1012, "sumitNonce request has bad 'nonce' parameter");
    return;
  }

  recip_id = wallet_->get_reward_recipient(account_id);
  if (!recip_id) {
    writeJSONError(1004, "Account's reward recipient doesn't match the pool's");
    return;
  }

  Block current_block;
  wallet_->get_current_block(current_block);

  std::shared_ptr<CalcDeadlineReq> cdr(new CalcDeadlineReq);
  cdr->account_id = account_id;
  cdr->nonce = nonce;
  cdr->height = current_block.height_;
  cdr->scoop_nr = current_block.scoop_;
  cdr->base_target = current_block.base_target_;
  cdr->gensig = current_block.gensig_;
  cdr->session = shared_from_this();
  cdr->recip_id = recip_id;

  deadline_req_handler_->enque_req(cdr);
}

void Session::writeJSONError(int error_code, std::string error_msg) {
  http::response<http::string_body> res{http::status::bad_request, req_.version()};
  res.body() = "{\"errorCode\":" + std::to_string(error_code) + ",\"errorDescription\":\"" + error_msg + "\"}";
  do_write(res);
}

void Session::write_ok(std::string& msg) {
  http::response<http::string_body> res{http::status::ok, req_.version()};
  res.body() = msg;
  do_write(res);
}

void Session::do_write(http::response<http::string_body>& res) {
  http::async_write(
      socket_,
      res,
      boost::asio::bind_executor(
          strand_,
          std::bind(
              &Session::on_write,
              shared_from_this(),
              std::placeholders::_1,
              std::placeholders::_2,
              true)));
}

void Session::on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close) {
  boost::ignore_unused(bytes_transferred);

  if(ec) {
    LOG(ERROR) << "write: " << ec.message();
    return;
  }

  if(close) {
    do_close();
    return;
  }

  res_ = nullptr;

  do_read();
}

void Session::do_close() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_send, ec);
}

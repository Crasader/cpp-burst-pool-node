#include "DeadlineRequestHandler.hpp"
#include <iostream>
#include "burstmath.h"

void DeadlineRequestHandler::enque_req(std::shared_ptr<CalcDeadlineReq> calc_deadline_req) {
  q_.enqueue(calc_deadline_req);
}

#ifdef USE_AVX2

void DeadlineRequestHandler::validate_deadlines(std::array<std::shared_ptr<CalcDeadlineReq>, PARALLEL_VALIDATIONS> creqs, int pending) {
  uint64_t deadlines[PARALLEL_VALIDATIONS];
  calculate_deadlines_avx2(
      creqs[0]->height >= poc2_start_height_,

      creqs[0]->base_target, creqs[1]->base_target, creqs[2]->base_target, creqs[3]->base_target,
      creqs[4]->base_target, creqs[5]->base_target, creqs[6]->base_target, creqs[7]->base_target,

      creqs[0]->scoop_nr, creqs[1]->scoop_nr, creqs[2]->scoop_nr, creqs[3]->scoop_nr,
      creqs[4]->scoop_nr, creqs[5]->scoop_nr, creqs[6]->scoop_nr, creqs[7]->scoop_nr,

      &creqs[0]->gensig[0], &creqs[1]->gensig[0], &creqs[2]->gensig[0], &creqs[3]->gensig[0],
      &creqs[4]->gensig[0], &creqs[5]->gensig[0], &creqs[6]->gensig[0], &creqs[7]->gensig[0],

      creqs[0]->account_id, creqs[1]->account_id, creqs[2]->account_id, creqs[3]->account_id,
      creqs[4]->account_id, creqs[5]->account_id, creqs[6]->account_id, creqs[7]->account_id,

      creqs[0]->nonce, creqs[1]->nonce, creqs[2]->nonce, creqs[3]->nonce,
      creqs[4]->nonce, creqs[5]->nonce, creqs[6]->nonce, creqs[7]->nonce,

      deadlines);

  for (int i = 0; i < pending; i++)
    validate_deadline(creqs[i], deadlines[i]);
}

#else

void DeadlineRequestHandler::validate_deadlines(std::array<std::shared_ptr<CalcDeadlineReq>, PARALLEL_VALIDATIONS> creqs, int pending) {
  uint64_t deadlines[PARALLEL_VALIDATIONS];
  calculate_deadlines_sse4(
      creqs[0]->height >= poc2_start_height_,

      creqs[0]->base_target, creqs[1]->base_target, creqs[2]->base_target, creqs[3]->base_target,

      creqs[0]->scoop_nr, creqs[1]->scoop_nr, creqs[2]->scoop_nr, creqs[3]->scoop_nr,

      &creqs[0]->gensig[0], &creqs[1]->gensig[0], &creqs[2]->gensig[0], &creqs[3]->gensig[0],

      creqs[0]->account_id, creqs[1]->account_id, creqs[2]->account_id, creqs[3]->account_id,

      creqs[0]->nonce, creqs[1]->nonce, creqs[2]->nonce, creqs[3]->nonce,

      deadlines);

  for (int i = 0; i < pending; i++)
    validate_deadline(creqs[i], deadlines[i]);
}

#endif

void DeadlineRequestHandler::validate_deadline(std::shared_ptr<CalcDeadlineReq> creq, uint64_t deadline) {
  if (deadline > deadline_limit_) {
    creq->session->writeJSONError(1008,  "Deadline exceeds deadline limit of the pool");
    return;
  }
  auto miner_round = wallet_->get_miner_round(creq->account_id);

  if (miner_round != nullptr && miner_round->mu.try_lock()) {
    if (miner_round->height == creq->height && miner_round->deadline > deadline) {
      nodecom::SubmitNonceRequest req;
      req.set_accountid(creq->account_id);
      req.set_nonce(creq->nonce);
      req.set_deadline(deadline);
      req.set_blockheight(creq->height);
      node_com_client_->SubmitNonce(req);
      miner_round->deadline = deadline;
    }
    miner_round->mu.unlock();
  }

  std::string msg = "{\"result\":\"success\",\"deadline\":" + std::to_string(deadline) + "}";
  creq->session->write_ok(msg);
}

void DeadlineRequestHandler::distribute_deadline_reqs() {
  std::array<std::shared_ptr<CalcDeadlineReq>, PARALLEL_VALIDATIONS> reqs;
  for (int i = 0; i < reqs.size(); i++) {
    std::shared_ptr<CalcDeadlineReq> req(new CalcDeadlineReq);
    req->base_target = 1;
    reqs[i] = req;
  }

  int pending = 0;
  int waited = 0;
  for (;;) {
    if (pending == PARALLEL_VALIDATIONS || (pending > 0 && waited > 1000)) {
      io_service_.post(boost::bind(&DeadlineRequestHandler::validate_deadlines, this, reqs, pending));
      pending = 0;
      waited = 0;
    }

    std::shared_ptr<CalcDeadlineReq> req;
    bool found = q_.try_dequeue(reqs[pending]);
    if (!found) {
      waited++;
      usleep(1000);
      continue;
    }

    pending++;
  }
}

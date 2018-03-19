#include "DeadlineRequestHandler.hpp"
#include "burstmath.h"
#include <iostream>
#include <mutex>

void DeadlineRequestHandler::add_req(CalcDeadlineReq * req) {
    _q.enqueue(req);
}

uint64_t DeadlineRequestHandler::calculate_deadline(uint64_t account_id, uint64_t nonce, uint64_t base_target,
                                                    uint32_t scoop_nr, uint8_t *gensig) {
    uint64_t deadline;
    CalcDeadlineReq calc_deadline_req;
    calc_deadline_req.account_id = account_id;
    calc_deadline_req.nonce = nonce;
    calc_deadline_req.scoop_nr = scoop_nr;
    calc_deadline_req.base_target = base_target;
    calc_deadline_req.gensig = gensig;

    add_req(&calc_deadline_req);
    {
        std::unique_lock<std::mutex> lock(calc_deadline_req.mu);
        calc_deadline_req.cv.wait(lock, [&calc_deadline_req]{return calc_deadline_req.processed;});
    }

    deadline = calc_deadline_req.deadline;

    return deadline;
}

void calculate_deadlines(std::vector<CalcDeadlineReq *> reqs, int pending) {
    std::vector<std::unique_lock<std::mutex> > locks;
    for (int i = 0; i < pending; i++)
        locks.push_back(std::unique_lock<std::mutex>(reqs[i]->mu));

    calculate_deadlines_avx2(
        0,

        reqs[0]->base_target, reqs[1]->base_target, reqs[2]->base_target, reqs[3]->base_target,
        reqs[4]->base_target, reqs[5]->base_target, reqs[6]->base_target, reqs[7]->base_target,

        reqs[0]->scoop_nr, reqs[1]->scoop_nr, reqs[2]->scoop_nr, reqs[3]->scoop_nr,
        reqs[4]->scoop_nr, reqs[5]->scoop_nr, reqs[6]->scoop_nr, reqs[7]->scoop_nr,

        reqs[0]->gensig, reqs[1]->gensig, reqs[2]->gensig, reqs[3]->gensig,
        reqs[4]->gensig, reqs[5]->gensig, reqs[6]->gensig, reqs[7]->gensig,

        reqs[0]->account_id, reqs[1]->account_id, reqs[2]->account_id, reqs[3]->account_id,
        reqs[4]->account_id, reqs[5]->account_id, reqs[6]->account_id, reqs[7]->account_id,

        reqs[0]->nonce, reqs[1]->nonce, reqs[2]->nonce, reqs[3]->nonce,
        reqs[4]->nonce, reqs[5]->nonce, reqs[6]->nonce, reqs[7]->nonce,

        &reqs[0]->deadline, &reqs[1]->deadline, &reqs[2]->deadline, &reqs[3]->deadline,
        &reqs[4]->deadline, &reqs[5]->deadline, &reqs[6]->deadline, &reqs[7]->deadline
    );

    for (int i = 0; i < pending; i++) {
        locks[i].unlock();
        reqs[i]->processed = true;
        reqs[i]->cv.notify_one();
    }
}

void DeadlineRequestHandler::distribute_deadline_reqs() {
    std::vector<CalcDeadlineReq *> reqs;

    for (int i = 0; i < 8; i++) {
        CalcDeadlineReq *req = new CalcDeadlineReq;
        // dummy gensig to avoid segfault
        req->gensig = (uint8_t*) malloc(32*sizeof(uint8_t));
        req->base_target = 1;
        reqs.push_back(req);
    }

    int pending = 0;
    int waited = 0;
    while (1) {
        if (pending == 8 || (pending > 0 && waited > 1000)) {
            _io_service.post(boost::bind(&calculate_deadlines, reqs, pending));

            pending = 0;
            waited = 0;
        }

        bool found = _q.try_dequeue(reqs[pending]);
        if (!found) {
            waited++;
            usleep(1000);
            continue;
        }

        pending++;
    }
}

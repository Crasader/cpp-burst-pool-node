#include "DeadlineRequestHandler.hpp"
#include "burstmath.h"
#include <iostream>
#include <mutex>

void DeadlineRequestHandler::add_req(CalcDeadlineReq * req) {
    _q.enqueue(req);
}

uint64_t DeadlineRequestHandler::calculate_deadline(uint64_t account_id, uint64_t nonce) {
    uint64_t deadline;
    CalcDeadlineReq *calc_deadline_req = new CalcDeadlineReq ;
    calc_deadline_req->account_id = account_id;
    calc_deadline_req->nonce = nonce;

    add_req(calc_deadline_req);
    {
        std::unique_lock<std::mutex> lock(calc_deadline_req->mu);
        calc_deadline_req->cv.wait(lock, [&calc_deadline_req]{return calc_deadline_req->processed;});
    }

    deadline = calc_deadline_req->deadline;

    delete calc_deadline_req;

    return deadline;
}

void calculate_deadlines(std::vector<CalcDeadlineReq *> reqs, int pending) {
    std::string gen_sig = "35844ab83e21851b38340cd7e8fc96b8bc139c132759ce3de1fcb616d888f2c9";

    std::vector<std::unique_lock<std::mutex> > locks;
    for (int i = 0; i < pending; i++)
        locks.push_back(std::unique_lock<std::mutex>(reqs[i]->mu));

    calculate_deadlines_avx2(1337, 1337, (uint8_t *) gen_sig.c_str(), 0,
        reqs[0]->account_id, reqs[1]->account_id, reqs[2]->account_id, reqs[3]->account_id,
        reqs[4]->account_id, reqs[5]->account_id, reqs[6]->account_id, reqs[7]->account_id,

        reqs[0]->nonce, reqs[1]->nonce, reqs[2]->nonce, reqs[3]->nonce,
        reqs[4]->nonce, reqs[5]->nonce, reqs[6]->nonce, reqs[7]->nonce,

        &reqs[0]->deadline, &reqs[1]->deadline, &reqs[2]->deadline, &reqs[3]->deadline,
        &reqs[4]->deadline, &reqs[5]->deadline, &reqs[6]->deadline, &reqs[7]->deadline
    );

    for (int i = 0; i < pending; i++)
        locks[i].unlock();

    for (int i = 0; i < pending; i++) {
        reqs[i]->processed = true;
        reqs[i]->cv.notify_one();
    }
}

void DeadlineRequestHandler::distribute_deadline_reqs() {
    std::vector<CalcDeadlineReq *> reqs;

    for (int i = 0; i < 8; i++) {
        CalcDeadlineReq *req = new CalcDeadlineReq;
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

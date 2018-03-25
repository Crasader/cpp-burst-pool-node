#include "DeadlineRequestHandler.hpp"
#include "burstmath.h"
#include <iostream>

void DeadlineRequestHandler::calculate_deadline(uint64_t account_id, uint64_t nonce, uint64_t base_target,
                                                    uint32_t scoop_nr, uint8_t *gensig, evhtp_request_t *req) {
    uint64_t deadline;
    CalcDeadlineReq *calc_deadline_req = new CalcDeadlineReq;
    calc_deadline_req->account_id = account_id;
    calc_deadline_req->nonce = nonce;
    calc_deadline_req->scoop_nr = scoop_nr;
    calc_deadline_req->base_target = base_target;
    calc_deadline_req->gensig = gensig;
    calc_deadline_req->req = req;

    _q.enqueue(calc_deadline_req);
}

void calculate_deadlines(std::array<CalcDeadlineReq *, 8> reqs, int pending) {
    int idxs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < pending; i++)
        idxs[i] = i;

    calculate_deadlines_avx2(
        0,

        reqs[idxs[0]]->base_target, reqs[idxs[1]]->base_target, reqs[idxs[2]]->base_target, reqs[idxs[3]]->base_target,
        reqs[idxs[4]]->base_target, reqs[idxs[5]]->base_target, reqs[idxs[6]]->base_target, reqs[idxs[7]]->base_target,

        reqs[idxs[0]]->scoop_nr, reqs[idxs[1]]->scoop_nr, reqs[idxs[2]]->scoop_nr, reqs[idxs[3]]->scoop_nr,
        reqs[idxs[4]]->scoop_nr, reqs[idxs[5]]->scoop_nr, reqs[idxs[6]]->scoop_nr, reqs[idxs[7]]->scoop_nr,

        reqs[idxs[0]]->gensig, reqs[idxs[1]]->gensig, reqs[idxs[2]]->gensig, reqs[idxs[3]]->gensig,
        reqs[idxs[4]]->gensig, reqs[idxs[5]]->gensig, reqs[idxs[6]]->gensig, reqs[idxs[7]]->gensig,

        reqs[idxs[0]]->account_id, reqs[idxs[1]]->account_id, reqs[idxs[2]]->account_id, reqs[idxs[3]]->account_id,
        reqs[idxs[4]]->account_id, reqs[idxs[5]]->account_id, reqs[idxs[6]]->account_id, reqs[idxs[7]]->account_id,

        reqs[idxs[0]]->nonce, reqs[idxs[1]]->nonce, reqs[idxs[2]]->nonce, reqs[idxs[3]]->nonce,
        reqs[idxs[4]]->nonce, reqs[idxs[5]]->nonce, reqs[idxs[6]]->nonce, reqs[idxs[7]]->nonce,

        &reqs[idxs[0]]->deadline, &reqs[idxs[1]]->deadline, &reqs[idxs[2]]->deadline, &reqs[idxs[3]]->deadline,
        &reqs[idxs[4]]->deadline, &reqs[idxs[5]]->deadline, &reqs[idxs[6]]->deadline, &reqs[idxs[7]]->deadline
    );

    for (int i = 0; i < pending; i++) {
        evbuffer_add_printf(reqs[i]->req->buffer_out, "{\"result\":\"success\",\"deadline\":%lu}",
                            reqs[i]->deadline);
        evhtp_send_reply(reqs[i]->req, EVHTP_RES_OK);
        delete reqs[i];
    }
}

void DeadlineRequestHandler::distribute_deadline_reqs() {
    std::array<CalcDeadlineReq *, 8> reqs;
    int pending = 0;
    int waited = 0;
    for (;;) {
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

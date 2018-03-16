#include <iostream>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/system/config.hpp>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <memory>
#include <algorithm>
#include "burstmath.h"
#include "DeadlineRequestHandler.hpp"

// #define NUM_VALIDATOR_THREADS 2

// struct CalcDeadlineReq {
//     uint64_t account_id;
//     uint64_t nonce;
//     uint64_t deadline;
//     std::condition_variable cv;
// };

// boost::lockfree::queue<std::shared_ptr<CalcDeadlineReq>, boost::lockfree::fixed_sized<true> > q{10000};
// boost::asio::io_service io_service;
// boost::thread_group threadpool;

// void calculate_deadlines(std::vector<std::shared_ptr<CalcDeadlineReq> > reqs, int pending) {
//     std::string gen_sig = "35844ab83e21851b38340cd7e8fc96b8bc139c132759ce3de1fcb616d888f2c9";

//     calculate_deadlines_avx2(1337, 1337, (uint8_t *) gen_sig.c_str(), 0,
//         (*reqs[0]).account_id, (*reqs[1]).account_id, (*reqs[2]).account_id, (*reqs[3]).account_id,
//         (*reqs[4]).account_id, (*reqs[5]).account_id, (*reqs[6]).account_id, (*reqs[7]).account_id,

//         (*reqs[0]).nonce, (*reqs[1]).nonce, (*reqs[2]).nonce, (*reqs[3]).nonce,
//         (*reqs[4]).nonce, (*reqs[5]).nonce, (*reqs[6]).nonce, (*reqs[7]).nonce,

//         &(*reqs[0]).deadline, &(*reqs[1]).deadline, &(*reqs[2]).deadline, &(*reqs[3]).deadline,
//         &(*reqs[4]).deadline, &(*reqs[5]).deadline, &(*reqs[6]).deadline, &(*reqs[7]).deadline
//     );

//     for (int i = 0; i < pending; i++)
//         (*reqs[i]).cv.notify_one();
// }

// void distribute_deadline_reqs() {
//     int pending = 0;
//     std::vector<std::shared_ptr<CalcDeadlineReq> > reqs;

//     for (int i = 0; i < 8; i++) {
//         std::shared_ptr<CalcDeadlineReq> req(new CalcDeadlineReq);
//         reqs.push_back(req);
//     }

//     std::shared_ptr<CalcDeadlineReq> test_req(new CalcDeadlineReq{1, 1, 1});
//     q.push(test_req);

//     int waited = 0;
//     while (1) {
//         if (pending == 8 || waited > 1000) {
//             io_service.post(boost::bind(&calculate_deadlines, reqs, pending));
//             pending = 0;
//             waited = 0;
//         }

//         if (q.empty()) {
//             waited++;
//             usleep(100);
//             continue;
//         }

//         q.pop(reqs[pending]);
//         pending++;
//     }
// }

int main () {
    DeadlineRequestHandler deadline_req_handler;

    std::shared_ptr<CalcDeadlineReq> test_req(new CalcDeadlineReq{1, 1, 1});

    for (int i = 0; i < 1000; i++) {
        deadline_req_handler.add_req(test_req);
    }

    usleep(100000000LL);
    
    // for (int i = 0; i < NUM_VALIDATOR_THREADS; i++)
    //     threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));

    // distribute_deadline_reqs();
}


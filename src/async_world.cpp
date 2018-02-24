// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#include "p/async/async_world.h"
#include <thread>

namespace p {
namespace async {

bool AsyncWorld::get_message(AsyncMessage* msg) {
    std::unique_lock<std::mutex> lock_gaurd(mutex_);
    while (q_.empty()) {
        condition_.wait(lock_gaurd);
    }
    *msg = q_.front();
    q_.pop_front();
    return true;
}

int AsyncWorld::push_message(AsyncMessage::Func func, void* object) {
    AsyncMessage msg{func, object};
    std::unique_lock<std::mutex> lock_gaurd(mutex_);
    q_.push_back(msg);
    condition_.notify_one();
    return q_.size();
}

class AsyncWorker {
public:
    AsyncWorker(AsyncWorld* async_world) : async_world_(async_world) {
        this_thread_ = std::thread(&AsyncWorker::worker_running, this);
    }

    void worker_running() {
        AsyncMessage message;
        while (true) {
            async_world_->get_message(&message);
            message.function_(message.object_);
        }
    }

private:
    AsyncWorld*                 async_world_;
    std::thread                 this_thread_;
};

AsyncWorld::AsyncWorld(const SharedQueueOptions& options) : thread_number_(options.thread_number) {
    work_list_ = new AsyncWorker*[thread_number_];
    for (size_t i = 0; i < thread_number_; ++i) {
        work_list_[i] = new AsyncWorker(this);
    }
}

//AsyncWorld::AsyncWorld(const ThreadedQueueOptions& options) {

} // end namespace async
} // end namespace p


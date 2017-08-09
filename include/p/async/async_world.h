// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace p {
namespace async {

struct AsyncMessage {
public:
    typedef void (*Func)(void*);
    Func        function_;
    void*       object_;
};

class AsyncWorker;

struct SharedQueueOptions {
    size_t  thread_number;
};

struct ThreadedQueueOptions {
    size_t  thread_number;
};

class AsyncWorld {
public:
    AsyncWorld(const SharedQueueOptions& options);

    bool get_message(AsyncMessage* msg);

    int  push_message(AsyncMessage::Func func, void* object);

    template<typename Object>
    int  push_message(void (Object::*func)(void), Object* object) {
        AsyncMessage::Func* f = (AsyncMessage::Func*)(&func);
        return push_message(*f, (void*)object);
    }

private:
    size_t                  thread_number_;
    std::mutex              mutex_;
    std::condition_variable condition_;

    //std::atomic<size_t>     size_;
    std::deque<AsyncMessage>          q_;

    AsyncWorker**             work_list_;
};

} // end namespace async
} // end namespace p


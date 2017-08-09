// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#pragma once

#include "p/async/socket.h"
#include "p/async/poller.h"
#include "p/base/socket.h"
#include <mutex>
#include <memory>
#include <unordered_set>

namespace p {
namespace async {

class Accepter {
public:
    Accepter(Poller* poller) : poller_(poller) {
        socket_ = Socket::acquire(-1); // set invliad fd
    }

    int listen(const base::EndPoint& ep) {
        int ret = socket_->listen(ep);
        if (!ret) {
            socket_->set_on_in_message(&Accepter::accept, this);
            poller_->add_socket(socket_);
        }
        return ret;
    }

    void accept();

    void add_socket(Socket* s) {
        s->accepter_ = this;
        poller_->add_socket(s);
        std::unique_lock<std::mutex>    lock_guard(mutex_);
        socket_map_.insert(s->socket_id());
    }

    void del_socket(Socket* s) {
        poller_->del_socket(s);
        {
            std::unique_lock<std::mutex>    lock_guard(mutex_);
            socket_map_.erase(s->socket_id());
        }
        s->release();
    }

private:
    Poller*                         poller_;
    Socket*                         socket_ = nullptr;
    //int32_t                         idle_timeout_sec_;

    std::mutex                      mutex_;
    std::unordered_set<SocketId>    socket_map_;
};

} // end namespace async
} // end namespace p


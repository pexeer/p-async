// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#include "p/async/accepter.h"
#include "p/base/logging.h"

namespace p {
namespace async {

void Accepter::accept() {
    while (true) {
        Socket* new_s = socket_->accept();
        if (new_s) {
            LOG_INFO << "new socket accpeted, remote=" << new_s->remote_side_;
            poller_->add_socket(new_s);
            std::unique_lock<std::mutex>    lock_guard(mutex_);
            socket_map_.insert(new_s->socket_id());
            continue;
        }

        break;
    }
}

} // end namespace async
} // end namespace p


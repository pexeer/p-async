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
            //new_s->set_on_in_message(&Accepter::on_in_message, new_s);
            //new_s->set_on_out_message(&Accepter::on_out_message, new_s);
            LOG_INFO << "new socket accpeted, remote=" << new_s->remote_side_;
            add_socket(new_s);
            continue;
        }

        break;
    }
}

} // end namespace async
} // end namespace p


// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#include "p/async/accepter.h"
#include "p/base/logging.h"

namespace p {
namespace async {

void Accepter::accept(Socket* s) {
    Accepter* accepter = (Accepter*) s->user();
    while (true) {
        Socket* new_s = s->accept();
        if (new_s) {
            new_s->set_on_in_message(&Accepter::on_message_in);
            new_s->set_on_out_message(&Accepter::on_message_out);
            LOG_INFO << "new socket accpeted, remote=" << new_s->remote_side_;
            new_s->set_user(accepter);
            accepter->add_socket(new_s);
            continue;
        }
        break;
    }
}

void Accepter::on_message_in(Socket* s) {
    while (true) {
        Socket* new_s = s->accept();
        if (new_s) {
            new_s->set_on_in_message(&Accepter::on_message_in);
            new_s->set_on_out_message(&Accepter::on_message_out);
            LOG_INFO << "new socket accpeted, remote=" << new_s->remote_side_;
            new_s->set_user(this);
            add_socket(new_s);
            continue;
        }
        break;
    }
}

void Accepter::on_message_out(Socket* s) {

}

void Accepter::on_failed(Socket* s) {

}

} // end namespace async
} // end namespace p


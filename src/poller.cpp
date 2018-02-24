// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#include "p/async/poller.h"
#include "p/async/socket.h"
#include "p/base/logging.h"
#include <unistd.h>             // pipe
#include <errno.h>              // errno
#include <sys/epoll.h>          // epoll_create

namespace p {
namespace async {

Poller::Poller(AsyncWorld* async_world) : async_world_(async_world) {
    poll_fd_ = ::epoll_create(/*ignore*/ 102400);
    if (poll_fd_ < 0) {
        //LOG_WARN << "Poller create epoll failed for " << strerror(errno);
    }

    if (::pipe(pipe_fds_) < 0) {
        //LOG_WARN << "Poller create pipe failed for " << strerror(errno);
    }

    async_world_->push_message(&Poller::poll, this);
}

Poller::~Poller() {
    if (poll_fd_ >= 0) {
        ::close(poll_fd_);
    }

    if (pipe_fds_[0] >= 0) {
        ::close(pipe_fds_[0]);
    }

    if (pipe_fds_[1] >= 0) {
        ::close(pipe_fds_[1]);
    }
}

int Poller::stop() {
    if (poll_fd_ <= 0) {
        return -1;
    }

    struct epoll_event evt = {EPOLLOUT, {nullptr}};
    ::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, pipe_fds_[1], &evt);

    return 0;
}

int Poller::start() {

    return 0;
}

void Poller::poll() {
    const int kMaxEventNum = 64;
    struct epoll_event evts[kMaxEventNum];

    const int n = ::epoll_wait(poll_fd_, evts, kMaxEventNum, -1);
    LOG_INFO << "Poller epoll_wait return " << n;

    for (int i = 0; i< n; ++i) {
        Socket* s = (Socket*)(evts[i].data.u64);
        if (evts[i].events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
            //async_world_->push_message(&Socket::on_poll_out, s->socket_id());
            async_world_->push_message(&Socket::on_poll_out, s);
        } else {
            //async_world_->push_message(&Socket::on_poll_in, s->socket_id());
            async_world_->push_message(&Socket::on_poll_in, s);
        }
    }

    async_world_->push_message(&Poller::poll, this);
}

int Poller::add_socket(Socket* s) {
    struct epoll_event evt;

    evt.data.u64 = (uint64_t)s;
    evt.events = EPOLLET | EPOLLIN;

    return ::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, s->fd(), &evt);
}

int Poller::mod_socket(Socket* s, bool add) {
    struct epoll_event evt;

    evt.data.u64 = (uint64_t)s;
    evt.events = EPOLLET | EPOLLIN;
    if (add) {
        evt.events |= EPOLLOUT;
    }

    return ::epoll_ctl(poll_fd_, EPOLL_CTL_MOD, s->fd(), &evt);
}

int Poller::del_socket(Socket* s) {
    return ::epoll_ctl(poll_fd_, EPOLL_CTL_DEL, s->fd(), nullptr);
}

} // end namespace async
} // end namespace p


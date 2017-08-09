// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#include "p/async/socket.h"
#include "p/base/endpoint.h"
#include <errno.h>  // errno
#include <fcntl.h>  // fctnl
#include <string.h> // memset
#include <string>

#include <netinet/in.h>  // IPPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELAY
#include <sys/socket.h>  // setsockopt

namespace p {
namespace async {

// set non-block flag for fd_
inline int Socket::set_non_block() {
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }

    if (!(flags & O_NONBLOCK)) {
        return ::fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
    }

    return flags;
}

// set close-on-exec flag for fd_
inline int Socket::set_close_on_exec() {
    int flags = ::fcntl(fd_, F_GETFD, 0);
    if (flags < 0) {
        return flags;
    }

    if (!(flags & FD_CLOEXEC)) {
        return ::fcntl(fd_, F_SETFD, flags | FD_CLOEXEC);
    }

    return flags;
}

inline int Socket::set_no_delay() {
    int flag = 1;
    return setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
}

int Socket::connect(const base::EndPoint &endpoint) {
    fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
        return errno;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(endpoint.port());
    servaddr.sin_addr.s_addr = endpoint.ip();

    set_non_block();
    set_close_on_exec();
    if (::connect(fd_, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
        return errno;
    }
    return 0;
}

int Socket::listen(const base::EndPoint &endpoint) {
    fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
        return errno;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(endpoint.port());
    servaddr.sin_addr.s_addr = endpoint.ip();
    if (::bind(fd_, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
        LOG_WARN << "bind " << endpoint << " failed for error=" << strerror(errno);
        return errno;
    }

    set_non_block();
    set_close_on_exec();

    if (::listen(fd_, SOMAXCONN) < 0) {
        LOG_WARN << "listen " << endpoint << " failed for error=" << strerror(errno);
        return errno;
    }
    return 0;
}

Socket* Socket::accept() {
    struct sockaddr new_addr;
    socklen_t addrlen = static_cast<socklen_t>(sizeof(new_addr));

    int fd = -1;
#ifdef P_OS_MACOSX
    fd = ::accept(fd_, &new_addr, &addrlen);
#else
    fd = ::accept4(fd_, &new_addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif

    if (fd < 0) {
        return nullptr;
    }

    Socket* new_s = Socket::acquire(fd);

#ifdef P_OS_MACOSX
    new_s->set_non_block();
    new_s->set_close_on_exec();
#endif

    new_s->local_side_ = local_side_;
    new_s->remote_side_ = base::EndPoint((struct sockaddr_in*)&new_addr);
    new_s->set_on_in_message(&Socket::on_in_message, new_s);
    new_s->set_on_out_message(&Socket::on_out_message, new_s);

    return new_s;
}

void Socket::on_in_message() {
    LOG_INFO << " on_in_message ";
    char buf[1024];
    while (true) {
        auto ret = ::read(fd_, buf, 1024);
        if (ret > 0) {
            std::string tmp(buf, ret);
            LOG_INFO << ">" << tmp;
            continue;
        }

        if (!ret) {
            LOG_INFO << "EOF for socket=" << this;
            break;
        }

        // ret < 0
        if (errno == EAGAIN) {
            break;
        }

        LOG_WARN << "ERROR for socket=" << this << ",error=" << strerror(errno);
        return ;
    }

    LOG_INFO << "on_in_message read end " << this;
}

void Socket::on_out_message() {
    LOG_INFO << " on_out_message ";
}


} // end namespace async
} // end namespace p

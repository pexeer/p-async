// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#pragma once

#include "p/base/macros.h"
#include "p/base/endpoint.h"
#include "p/base/object_pool.h"
#include "p/async/async_world.h"

namespace p {
namespace async {

typedef     uint64_t    SocketId;

class UniqueSocketPtr;
class Accepter;
class Poller;

class P_CACHELINE_ALIGNMENT  Socket {
public:
    Socket() {}

    SocketId socket_id() const {
        return socket_id_;
    }

    base::EndPoint remote_side() const {
        return remote_side_;
    }

    base::EndPoint local_side() const {
        return local_side_;
    }

    int fd() {
        return fd_;
    }

    int connect(const base::EndPoint &endpoint);

    int listen(const base::EndPoint &endpoint);

    Socket* accept();

    int set_non_block();

    int set_close_on_exec();

    int set_no_delay();

    template<typename Function>
    void set_on_in_message(Function f, void* user) {
        AsyncMessage::Func* tmp = (AsyncMessage::Func*)&f;
        on_in_message_ = *tmp;
        in_user_ = user;
    }

    template<typename Function>
    void set_on_out_message(Function f, void* user) {
        AsyncMessage::Func* tmp = (AsyncMessage::Func*)&f;
        on_out_message_ = *tmp;
        out_user_ = user;
    }

    void on_in_message();

    void on_out_message();

public:
    typedef base::ArenaObjectPool<Socket> SocketFactory;

    static Socket* acquire(int fd) {
        SocketId socket_id;
        Socket*  s = SocketFactory::acquire(&socket_id);
        s->socket_id_ = socket_id + 0x100000000;    // version + 1
        uint64_t vref = s->versioned_ref_.load(std::memory_order_acquire);
        assert((vref >> 32) == (socket_id >> 32));
        assert((vref & 0xFFFFFFFF) == 0);

        s->versioned_ref_.fetch_add(0x100000001, std::memory_order_release);
        s->fd_ = fd;

        return s;
    }

    static void release(Socket* s) {
        if (s->fd_) {
            ::close(s->fd_);
        }

        SocketFactory::release(s->socket_id_ + 0x100000000);
    }

    friend class UniqueSocketPtr;
    friend class Accepter;
    friend class Poller;
private:
    std::atomic<uint64_t>   versioned_ref_ = {0};

    std::atomic<int>        fd_ = {-1};

    // imutable after create
    SocketId            socket_id_;
    base::EndPoint      remote_side_;
    base::EndPoint      local_side_;

    AsyncMessage::Func  on_in_message_ = nullptr;
    void*               in_user_ = nullptr;

    AsyncMessage::Func  on_out_message_ = nullptr;
    void*               out_user_ = nullptr;
private:
    P_DISALLOW_COPY(Socket);
};

class UniqueSocketPtr {
public:
    UniqueSocketPtr(SocketId socket_id) {
        socket_ = Socket::SocketFactory::find(socket_id);
        if (socket_id != socket_->socket_id_) {
            socket_ = nullptr;
        }

        uint64_t old_vref = socket_->versioned_ref_.fetch_add(0x1, std::memory_order_release);

        if ((old_vref & 0xFFFFFFFF) == 0) {
            //socket_->versioned_ref_.
        }
    }

    ~UniqueSocketPtr() {
        if (socket_) {
            uint64_t old_vref = socket_->versioned_ref_.fetch_sub(0x1, std::memory_order_release);
            if ((old_vref & 0xFFFFFFFF) == 0) {
                Socket::release(socket_);
            }
        }
    }

    Socket* get() const {
        return socket_;
    }

    Socket* release() {
        Socket* ret = socket_;
        socket_ = nullptr;
        return ret;
    }

    void swap(UniqueSocketPtr* ptr) {
        Socket* ret = socket_;
        socket_ = ptr->socket_;
        ptr->socket_ = ret;
    }

    Socket* operator->() const {
        return socket_;
    }

    Socket*             socket_ = nullptr;
private:
    P_DISALLOW_COPY(UniqueSocketPtr);
};

} // end namespace async
} // end namespace p

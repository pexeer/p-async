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

inline uint64_t VersionOf(uint64_t id) {
    return id >> 32;
}

inline uint64_t RefOf(uint64_t id) {
    return id & 0xFFFFFFFF;
}

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
        uint64_t vref = s->versioned_ref_.load(std::memory_order_acquire);
        assert(VersionOf(vref) == VersionOf(socket_id));
        assert(RefOf(vref) == 0);

        s->fd_ = fd;
        // AddVersion
        s->socket_id_ = socket_id + 0x100000000;    // version + 1
        // AddVersion & AddRef
        s->versioned_ref_.fetch_add(0x100000001, std::memory_order_release);

        return s;
    }

    void release() {
        uint64_t old_vref = versioned_ref_.fetch_sub(0x1, std::memory_order_release);
        if (RefOf(old_vref) == 1) {
            LOG_TRACE << "Socket=" << this << ", destroy for socket_id=" << base::HexUint64(socket_id_);
            // release this
            if (fd_) {
                ::close(fd_);
            }

            assert(VersionOf(old_vref) == VersionOf(socket_id_) + 1);
            SocketFactory::release(socket_id_ + 0x100000000);
        }
    }

    bool ok() {
        uint64_t vref = versioned_ref_.load(std::memory_order_acquire);
        return VersionOf(vref) == VersionOf(socket_id_);
    }

    bool set_failed(int err) {
        while (true) {
            uint64_t vref = versioned_ref_.load(std::memory_order_acquire);
            if (VersionOf(vref) != VersionOf(socket_id_)) {
                return false;
            }
            if (versioned_ref_.compare_exchange_weak(vref,
                                                     vref + 0x100000000,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed)) {
                errno_ = err;
                return true;
            }
        }
    }

    friend class UniqueSocketPtr;
    friend class Accepter;
    friend class Poller;
private:
    std::atomic<uint64_t>   versioned_ref_ = {0};

    std::atomic<int>        fd_ = {-1};
    int                     errno_ = 0;

    // imutable after create
    SocketId            socket_id_;
    base::EndPoint      remote_side_;
    base::EndPoint      local_side_;

    AsyncMessage::Func  on_in_message_ = nullptr;
    void*               in_user_ = nullptr;

    AsyncMessage::Func  on_out_message_ = nullptr;
    void*               out_user_ = nullptr;

    Accepter*           accepter_ = nullptr;
private:
    P_DISALLOW_COPY(Socket);
};

class UniqueSocketPtr {
public:
    UniqueSocketPtr(SocketId socket_id) {
        socket_ = Socket::SocketFactory::find(socket_id);
        if (socket_id != socket_->socket_id_) {
            socket_ = nullptr;
            return ;
        }

        // AddRef
        uint64_t old_vref = socket_->versioned_ref_.fetch_add(0x1, std::memory_order_release);

        if (RefOf(old_vref) && VersionOf(old_vref) == VersionOf(socket_id)) {
            return ;
        }

        // Add wrong ref, need to do release
        socket_->release();
        socket_ = nullptr;
    }

    UniqueSocketPtr(Socket* s) : socket_(s) {
        uint64_t old_vref = socket_->versioned_ref_.fetch_add(0x1, std::memory_order_release);
        if (VersionOf(socket_->socket_id_) == VersionOf(old_vref) && RefOf(old_vref)) {
            return ;
        }

        socket_->versioned_ref_.fetch_sub(0x1, std::memory_order_release);
        socket_ = nullptr;
    }

    ~UniqueSocketPtr() {
        if (socket_) {
            socket_->release();
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

    explicit operator bool() const {
        return socket_;
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

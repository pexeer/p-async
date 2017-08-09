// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#pragma once

#include "p/async/async_world.h"

namespace p {
namespace async {

class Socket;

class Poller {
public:
  Poller(AsyncWorld* async_world);

  ~Poller();

  int start();

  void poll();

  int stop();

  int add_socket(Socket* s);

  int mod_socket(Socket* s, bool add);

  int del_socket(Socket* s);

protected:
  AsyncWorld*       async_world_;


  int poll_fd_ = -1;
  int pipe_fds_[2] = {-1, -1};
};

} // end namespace async
} // end namespace p


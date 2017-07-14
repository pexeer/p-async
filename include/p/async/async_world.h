// Copyright (c) 2017, pexeer@gmail.com All rights reserved.
// Licensed under a BSD-style license that can be found in the LICENSE file.

#pragma once

namespace p {
namespace async {

class AsyncWorld {
public:
    struct AsyncWorldOptions {
        int thread_pool_size;
    };

    AsyncWorld(AsyncWorldOptions options);

private:

};

} // end namespace async
} // end namespace p


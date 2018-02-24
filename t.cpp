#include <iostream>
#include <thread>

#include "p/async/poller.h"
#include "p/async/accepter.h"
#include "p/base/logging.h"

int main() {
    p::base::LogMessage::set_wf_log_min_level(p::base::LogLevel::kTrace);
    p::async::SharedQueueOptions  options;
    options.thread_number = 5;
    p::async::AsyncWorld async_world(options);
    p::async::Poller poll(&async_world);

    p::async::Accepter accepter(&poll);

    sleep(1);

    //p::base::EndPoint listen(p::base::EndPoint::local_ip(), 8001);
    p::base::EndPoint listen("0.0.0.0:8001");
    int ret = accepter.listen(listen);

    LOG_INFO << "listen ret=" << ret;

    sleep(100);
    return ret;
}

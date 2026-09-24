#include "adapters/http/RequestWorkers.hpp"

#include <utility>

#include <trantor/utils/Logger.h>

namespace communication::adapters::http {

RequestWorkers::RequestWorkers(std::size_t threads) {
    threads_.reserve(threads);
    for (std::size_t made = 0; made < threads; ++made) {
        threads_.emplace_back([this] { Run(); });
    }
}

RequestWorkers::~RequestWorkers() {
    {
        const std::lock_guard held(lock_);
        stopping_ = true;
    }
    wake_.notify_all();
    for (auto& thread : threads_) {
        thread.join();
    }
}

bool RequestWorkers::Submit(std::function<void()> work) {
    {
        const std::lock_guard held(lock_);
        if (stopping_) {
            return false;
        }
        work_.push(std::move(work));
    }
    wake_.notify_one();
    return true;
}

void RequestWorkers::Run() {
    while (true) {
        std::function<void()> next;
        {
            std::unique_lock held(lock_);
            wake_.wait(
                held,
                [this] {
                    return stopping_ || !work_.empty();
                }
            );

            if (work_.empty()) {
                return;
            }
            next = std::move(work_.front());
            work_.pop();
        }

        try {
            next();
        } catch (const std::exception& e) {
            LOG_ERROR << "a request handler threw: " << e.what();
        }
    }
}

}  // namespace communication::adapters::http

#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace communication::adapters::http {

class RequestWorkers {
  public:
    explicit RequestWorkers(std::size_t threads);
    ~RequestWorkers();

    RequestWorkers(const RequestWorkers&) = delete;
    RequestWorkers& operator=(const RequestWorkers&) = delete;
    RequestWorkers(RequestWorkers&&) = delete;
    RequestWorkers& operator=(RequestWorkers&&) = delete;

    [[nodiscard]] bool Submit(std::function<void()> work);

  private:
    void Run();

    std::mutex lock_;
    std::condition_variable wake_;
    std::queue<std::function<void()>> work_;
    std::vector<std::thread> threads_;
    bool stopping_ = false;
};

}  // namespace communication::adapters::http

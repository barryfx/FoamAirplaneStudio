#pragma once
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <vector>

namespace designrc::processing {
// Component-neutral owner: workers never capture a widget or invoke the viewer.
// The GUI drains copied status messages and consumes the result only when ready.
template<class Result>
class BackgroundJob {
public:
  using Progress=std::function<void(const char*)>;
  using Work=std::function<Result(std::stop_token,const Progress&)>;
  explicit BackgroundJob(Work work) {
    const auto messages=messages_;const auto token=stop_.get_token();
    result_=std::async(std::launch::async,[work=std::move(work),messages,token] {
      return work(token,[messages](const char* message) {
        std::lock_guard lock{messages->mutex};messages->pending.emplace_back(message);
      });
    });
  }
  ~BackgroundJob() { cancel();if(result_.valid())result_.wait(); }
  BackgroundJob(const BackgroundJob&)=delete;
  BackgroundJob& operator=(const BackgroundJob&)=delete;
  void cancel() { stop_.request_stop(); }
  bool cancelled() const { return stop_.stop_requested(); }
  bool ready() const { return result_.wait_for(std::chrono::seconds{0})==std::future_status::ready; }
  Result take() { return result_.get(); }
  std::vector<std::string> messages() {
    std::lock_guard lock{messages_->mutex};std::vector<std::string> result;
    result.swap(messages_->pending);return result;
  }
private:
  struct Messages {std::mutex mutex;std::vector<std::string> pending;};
  std::shared_ptr<Messages> messages_=std::make_shared<Messages>();
  std::stop_source stop_;
  std::future<Result> result_;
};
}

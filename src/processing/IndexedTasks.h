#pragma once
#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

namespace designrc::processing {
// Each task owns its result slot and CAD objects. No OCCT object is shared for
// mutation. Bound memory use independently of the number of panels/components.
template<class Work>
void runIndexedTasks(std::size_t count,Work work,std::stop_token external={},unsigned limit=0) {
  if(!count)return;
  if(!limit)limit=std::min(4u,std::max(1u,std::thread::hardware_concurrency()));
  const auto workers=std::min<std::size_t>(count,limit);
  std::stop_source cancel;
  std::stop_callback forward{external,[&]{cancel.request_stop();}};
  std::atomic_size_t next{0};std::mutex errorMutex;std::exception_ptr error;
  auto run=[&] {
    while(!cancel.stop_requested()) {
      const auto index=next.fetch_add(1);if(index>=count)break;
      try {work(index,cancel.get_token());}
      catch(...) {
        {std::lock_guard lock{errorMutex};if(!error)error=std::current_exception();}
        cancel.request_stop();break;
      }
    }
  };
  if(workers==1)run();
  else {
    std::vector<std::jthread> threads;threads.reserve(workers);
    // A failed thread allocation still joins already-started workers before
    // their borrowed inputs/results leave scope.
    try {for(std::size_t i=0;i<workers;++i)threads.emplace_back(run);}
    catch(...) {cancel.request_stop();throw;}
    for(auto& thread:threads)thread.join();
  }
  if(error)std::rethrow_exception(error);
}
}

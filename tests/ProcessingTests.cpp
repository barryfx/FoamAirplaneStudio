#include "processing/BackgroundJob.h"
#include "processing/IndexedTasks.h"
#include "geometry/ProcessingControl.h"
#include <barrier>
#include <array>
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <stdexcept>
using namespace designrc;
#define CHECK(c) do{if(!(c))throw std::runtime_error(#c);}while(false)
int main() {
  try {
    std::barrier pair{2};std::atomic_int active=0,peak=0,complete=0;
    processing::runIndexedTasks(6,[&](std::size_t,std::stop_token){
      const int count=++active;int previous=peak.load();while(count>previous && !peak.compare_exchange_weak(previous,count)){}
      pair.arrive_and_wait();CHECK(active==2);pair.arrive_and_wait();--active;++complete;
    },{},2);
    CHECK(peak==2 && complete==6);
    // Four component slots can all execute concurrently and publish independently.
    std::barrier components{4};std::array<int,4> results{};
    processing::runIndexedTasks(4,[&](std::size_t index,std::stop_token) {
      components.arrive_and_wait();results[index]=static_cast<int>(index+1);
    },{},4);
    CHECK((results==std::array<int,4>{1,2,3,4}));
    std::atomic_int joined=0;bool failed=false;std::barrier failingPair{2};
    try {processing::runIndexedTasks(10,[&](std::size_t index,std::stop_token stop){
      failingPair.arrive_and_wait();
      if(index==0)throw std::runtime_error("component failure");
      while(!stop.stop_requested())std::this_thread::yield();++joined;
    },{},2);}catch(const std::runtime_error& e){failed=std::string{e.what()}=="component failure";}
    CHECK(failed && joined==1);
    std::promise<void> started;auto start=started.get_future();
    processing::BackgroundJob<int> job{[&](std::stop_token stop,const auto& report){
      report("Wing panel 1: sample");started.set_value();
      std::mutex mutex;std::condition_variable_any condition;std::unique_lock lock{mutex};
      condition.wait(lock,stop,[]{return false;});geometry::ProcessingControl{stop}.checkpoint();return 42;
    }};
    CHECK(start.wait_for(std::chrono::seconds{5})==std::future_status::ready);
    CHECK(job.messages()==std::vector<std::string>{"Wing panel 1: sample"});job.cancel();
    bool cancelled=false;try{job.take();}catch(const geometry::ProcessingCancelled&){cancelled=true;}CHECK(cancelled);
    // The returned progress range must keep its indicator's parent scope alive.
    std::stop_source stop;geometry::ProcessingControl control{stop.get_token()};auto progress=control.range();
    CHECK(!static_cast<const Message_ProgressRange&>(progress).UserBreak());stop.request_stop();
    CHECK(static_cast<const Message_ProgressRange&>(progress).UserBreak());
    std::cout<<"Bounded concurrent tasks, failure joining, copied status messages and cooperative cancellation passed.\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

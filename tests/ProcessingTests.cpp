#include "processing/BackgroundJob.h"
#include "processing/IndexedTasks.h"
#include "geometry/ProcessingControl.h"
#include "geometry/FuselageProcessing.h"
#include "geometry/FuselageMaterialSpans.h"
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Trsf.hxx>
#include <cmath>
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
    const auto box=BRepPrimAPI_MakeBox{10,20,30}.Shape();
    const auto hollow=BRepAlgoAPI_Cut{box,BRepPrimAPI_MakeCylinder{gp_Ax2{gp_Pnt{5,10,-1},gp_Dir{0,0,1}},2,32}.Shape()}.Shape();
    gp_Trsf translation;translation.SetTranslation(gp_Vec{123,-456,789});
    for(const auto& shape:{box,hollow,hollow.Moved(TopLoc_Location{translation})}) {
      GProp_GProps original,incremental;BRepGProp::VolumeProperties(shape,original);
      geometry::fuselageVolumeProperties(shape,incremental,{});
      CHECK(std::abs(original.Mass()-incremental.Mass())<std::abs(original.Mass())*1e-9);
      CHECK(original.CentreOfMass().Distance(incremental.CentreOfMass())<1e-7);
      Bnd_Box a,b;BRepBndLib::AddOptimal(shape,a,false,false);geometry::fuselageBounds(shape,b,{});
      double aa[6],bb[6];a.Get(aa[0],aa[1],aa[2],aa[3],aa[4],aa[5]);b.Get(bb[0],bb[1],bb[2],bb[3],bb[4],bb[5]);
      for(int i=0;i<6;++i)CHECK(std::abs(aa[i]-bb[i])<1e-7);
    }
    const auto cavity=BRepAlgoAPI_Cut{box,BRepPrimAPI_MakeBox{gp_Pnt{2,2,2},6,16,26}.Shape()}.Shape();
    for(const auto& shape:{box,hollow,cavity}) {
      geometry::FuselageMaterialSpans spans{shape,-1,31,{}};
      BRepClass3d_SolidClassifier classifier{shape};
      for(double x:{1.3,5.,8.3})for(double y:{1.3,10.,17.3}) {
        const auto intervals=spans.at(x,y,{});
        for(double z:{-1.,.5,2.5,15.,28.5,31.}) {
          classifier.Perform(gp_Pnt{x,y,z},1e-7);
          const bool found=std::any_of(intervals.begin(),intervals.end(),[&](const auto& interval){return z>interval.first&&z<interval.second;});
          CHECK(found==(classifier.State()==TopAbs_IN));
        }
      }
    }
    BRep_Builder seamBuilder;TopoDS_Compound halves;seamBuilder.MakeCompound(halves);
    seamBuilder.Add(halves,BRepAlgoAPI_Common{cavity,BRepPrimAPI_MakeBox{gp_Pnt{-1,-1,-1},12,11,32}.Shape()}.Shape());
    seamBuilder.Add(halves,BRepAlgoAPI_Common{cavity,BRepPrimAPI_MakeBox{gp_Pnt{-1,10,-1},12,11,32}.Shape()}.Shape());
    geometry::FuselageMaterialSpans seam{halves,-1,31,{}};
    const auto intervals=seam.at(5,10,{});CHECK(intervals.size()==2);
    CHECK(std::abs(intervals[0].first)<1e-7&&std::abs(intervals[0].second-2)<1e-7);
    CHECK(std::abs(intervals[1].first-28)<1e-7&&std::abs(intervals[1].second-30)<1e-7);
    bool rayCancelled=false;try{seam.at(5,10,control);}catch(const geometry::ProcessingCancelled&){rayCancelled=true;}CHECK(rayCancelled);
    // Cancellation arrives while a many-face mass calculation is in progress.
    BRep_Builder builder;TopoDS_Compound many;builder.MakeCompound(many);
    for(int i=0;i<10000;++i)builder.Add(many,box);
    std::promise<void> massStarted;auto massStart=massStarted.get_future();
    processing::BackgroundJob<int> massJob{[&](std::stop_token token,const auto&) {
      massStarted.set_value();GProp_GProps properties;geometry::fuselageVolumeProperties(many,properties,{token});return 1;
    }};
    CHECK(massStart.wait_for(std::chrono::seconds{5})==std::future_status::ready);
    std::this_thread::sleep_for(std::chrono::milliseconds{10});massJob.cancel();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds{5};
    while(!massJob.ready()&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds{1});
    CHECK(massJob.ready());cancelled=false;try{massJob.take();}catch(const geometry::ProcessingCancelled&){cancelled=true;}CHECK(cancelled);
    std::cout<<"Bounded concurrent tasks, failure joining, copied status messages and cooperative cancellation passed.\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#pragma once
#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressRange.hxx>
#include <stop_token>
#include <stdexcept>
#include <utility>

namespace designrc::geometry {
class ProcessingCancelled : public std::runtime_error {
public:
  ProcessingCancelled() : std::runtime_error{"Processing cancelled."} {}
};
// Explicit, copyable operation context. Each OCCT call gets its own indicator;
// only the stop state is shared, so independent algorithms never share progress ranges.
struct ProcessingControl {
  std::stop_token stop;
  void checkpoint() const { if(stop.stop_requested())throw ProcessingCancelled{}; }
  struct OperationProgress {
    // OCCT ranges borrow their parent scope. Keep its indicator alive until the
    // range has been destroyed, including when an algorithm throws.
    Handle(Message_ProgressIndicator) indicator;
    Message_ProgressRange progress;
    explicit OperationProgress(Handle(Message_ProgressIndicator) value)
        : indicator{std::move(value)},progress{Message_ProgressIndicator::Start(indicator)} {}
    OperationProgress(const OperationProgress&)=delete;
    OperationProgress& operator=(const OperationProgress&)=delete;
    operator const Message_ProgressRange&() const { return progress; }
  };
  OperationProgress range() const {
    checkpoint();
    if(!stop.stop_possible())return OperationProgress{{}};
    class Indicator final : public Message_ProgressIndicator {
    public:
      explicit Indicator(std::stop_token token) : token_{token} {}
    protected:
      bool UserBreak() override { return token_.stop_requested(); }
      void Show(const Message_ProgressScope&,bool) override {}
    private:
      std::stop_token token_;
    };
    Handle(Message_ProgressIndicator) indicator=new Indicator{stop};
    return OperationProgress{std::move(indicator)};
  }
};
}

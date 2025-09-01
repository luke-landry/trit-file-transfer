
#include "WorkerContext.h"

bool WorkerContext::should_abort() const { return abort_flag_.load(); }

void WorkerContext::handle_exception() {
  abort_flag_ = true;
  std::lock_guard<std::mutex> lock(exception_mutex_);
  if (!exception_ptr_) {
    exception_ptr_ = std::current_exception();
  }
}

void WorkerContext::rethrow_if_exception() {
  std::lock_guard<std::mutex> lock(exception_mutex_);
  if (exception_ptr_) {
    std::rethrow_exception(exception_ptr_);
  }
}

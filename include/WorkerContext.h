#ifndef WORKER_CONTEXT_H
#define WORKER_CONTEXT_H

#include <atomic>
#include <exception>
#include <mutex>
#include <functional>

// Context for shared worker synchronization and error handling
class WorkerContext {
public:

    bool should_abort() const;
    
    // Capture the first exception thrown across any worker thread
    void handle_exception();

    void rethrow_if_exception();

private:
    std::atomic<bool> abort_flag_{false};
    std::mutex exception_mutex_;
    std::exception_ptr exception_ptr_ = nullptr;
};

#endif // WORKER_CONTEXT_H
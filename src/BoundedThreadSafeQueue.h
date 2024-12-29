#ifndef BOUNDED_THREAD_SAFE_QUEUE_H
#define BOUNDED_THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class BoundedThreadSafeQueue{
    public:
        BoundedThreadSafeQueue(size_t capacity): capacity_(capacity) {}

        void push(T&& elem){
            // Protect push operation with mutex, will be automatically released after push
            std::unique_lock<std::mutex> lock(mutex_);

            // Wait until the queue is not full
            not_full_.wait(lock, [this](){ return queue_.size() < capacity_; });

            // Push the forwarded elem (which could be lvalue or rvalue)
            queue_.push(std::forward<T>(elem));

            // Notify any threads waiting to pop that queue is no longer empty
            not_empty_.notify_one();
        }

        T pop(){
            // Protect pop operation with mutex, will be automatically released after pop
            std::lock_guard<std::mutex> lock(mutex_);

            // Wait until the queue is not empty
            not_empty_.wait(lock, [this](){ return queue_.size() > 0; });

            // Pop from queue
            T elem = queue_.front();
            queue_.pop();

            // Notify any threads waiting to push that the queue is no longer full
            not_full_.notify_one();

            return elem;
        }

        bool empty(){
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        bool full(){
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size() >= capacity_;
        }

        size_t size(){
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

    private:
        size_t capacity_;
        std::queue<T> queue_;
        std::mutex mutex_;
        std::condition_variable not_empty_;
        std::condition_variable not_full_;
};

#endif
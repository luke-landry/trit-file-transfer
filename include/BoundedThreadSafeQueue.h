#ifndef BOUNDED_THREAD_SAFE_QUEUE_H
#define BOUNDED_THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <stdexcept>

template<typename T>
class BoundedThreadSafeQueue{
    public:
        BoundedThreadSafeQueue(size_t capacity): capacity_(capacity) {
            if(capacity_ == 0){
                throw std::logic_error("Queue capacity must be greater than 0");
            }
        }

        // Only allowed passing rvalues to force not copying objects, only moving allowed
        void push(T&& elem){
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this](){ return queue_.size() < capacity_; });
            queue_.push(std::move(elem));
            not_empty_.notify_one();
        }

        template <typename... Args>
        void emplace(Args&&... args){
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this](){ return queue_.size() < capacity_; });
            queue_.emplace(std::forward<Args>(args)...);
            not_empty_.notify_one();
        }

        T pop(){
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_.wait(lock, [this](){ return queue_.size() > 0; });
            T elem = std::move(queue_.front());
            queue_.pop();
            not_full_.notify_one();
            return elem;
        }

        std::optional<T> try_pop(){
            std::unique_lock<std::mutex> lock(mutex_);
            if(queue_.empty()){ return std::nullopt; }
            T elem = std::move(queue_.front());
            queue_.pop();
            not_full_.notify_one();
            return elem;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        bool full() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size() >= capacity_;
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

    private:
        const size_t capacity_;
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable not_empty_;
        std::condition_variable not_full_;
};

#endif
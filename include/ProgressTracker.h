#ifndef PROGRESS_TRACKER_H
#define PROGRESS_TRACKER_H

#include <string>
#include <vector>
#include <atomic>

template <typename T>
class ProgressTracker {
    public:

        void track(std::string name, std::shared_ptr<std::atomic<T>>, T limit){
            static_assert(std::is_integral_v<T>, "Template type of progress tracker must be integral");
        }

        void start(){

        }

    private:
        struct TrackedValue {
                std::string name;
                std::shared_ptr<std::atomic<T>> value;
                T limit;
        };

        std::vector<TrackedValue> tracked_values_;


};

#endif
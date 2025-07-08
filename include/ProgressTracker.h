#ifndef PROGRESS_TRACKER_H
#define PROGRESS_TRACKER_H

#include <string>
#include <atomic>
#include <iomanip>

template <typename T>
class ProgressTracker {
    public:

        ProgressTracker(std::string name, T limit): name_(name), limit_(limit) {
            static_assert(std::is_integral_v<T>, "Template type of progress tracker must be integral");
            if(limit_ < 1){
                throw std::logic_error("Invalid progress tracker limit");
            }
        }

        void start(WorkerContext& ctx, const std::atomic<T>& value){
            bool done = false;

            // Max value of T being used as sentinel to indicate not initialized state
            // For progress tracking, T is expected to start at 0 and increase monotonically
            T last_value = std::numeric_limits<T>::max();
            while(true){
                if(ctx.should_abort()){
                    std::cout << std::endl;
                    return; 
                }
                
                T current_value = value.load();
                if(current_value != last_value){
                    double progress_percentage = static_cast<double>(current_value) / limit_ * 100.0;
                    const int bar_width = 30;
                    constexpr char progress_fill_char = '#';
                    constexpr char progress_empty_char = '-';
                    int progress_fill_num_chars = static_cast<int>((progress_percentage / 100.0) * bar_width);
                    int progress_empty_num_chars = bar_width - progress_fill_num_chars;

                    std::string progress_bar = "[";
                    progress_bar += std::string(progress_fill_num_chars, progress_fill_char);
                    progress_bar += std::string(progress_empty_num_chars, progress_empty_char);
                    progress_bar += "]";

                    std::ostringstream oss;
                    oss << "\r" << std::left << std::setw(20) << name_ << progress_bar
                        << " " << std::fixed << std::setprecision(1) << progress_percentage
                        << "% (" << current_value << "/" << limit_ << ")";
                        
                    std::cout << oss.str() << std::flush;
                    last_value = current_value;
                }
                    
                if(current_value >= limit_){ break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::cout << std::endl;
        }

    private:
        std::string name_;
        T limit_;
};

#endif
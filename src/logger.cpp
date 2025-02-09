
#include "logger.h"

#include <iostream>
#include <mutex>

#include "utils.h"

void logger::console(const std::string& tag, const std::string& message){
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << utils::get_timestamp("%H:%M:%S") << " [" << tag << "]\t" << message << std::endl;
}
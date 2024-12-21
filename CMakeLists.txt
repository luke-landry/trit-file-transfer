cmake_minimum_required(VERSION "3.10")

project(trit)

# Require C++17 for project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(ASIO_STANDALONE)

# Outputting final executable file into the /bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

add_executable(trit
    src/main.cpp
)

# Include Asio library based on platform
if(WIN32)
    message(STATUS "Configuring CMake for Windows")
    include_directories(C:/libraries/asio/include)
    target_link_libraries(${PROJECT_NAME} ws2_32 mswsock)

elseif(UNIX)
    message(STATUS "Configuring CMake for Linux")
    include_directories(/usr/include)

    # Threading libaries not linked by default on Linux, so must be included here
    target_link_libraries(${PROJECT_NAME} pthread)
endif()






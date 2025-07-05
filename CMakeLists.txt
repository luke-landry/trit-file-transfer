cmake_minimum_required(VERSION "3.10")

project(trit)

# Require C++17 for project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(ASIO_STANDALONE)

# Outputting final executable file into the /bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

include_directories(include)

find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

add_executable(trit
    src/main.cpp
    src/staging.cpp
    src/utils.cpp
    src/crypto.cpp
    src/Sender.cpp
    src/Receiver.cpp
    src/TransferRequest.cpp
    src/Chunk.cpp
    src/FileManager.cpp
    src/TransferManager.cpp
    src/CompressionManager.cpp
)

target_include_directories(trit PRIVATE ${LIBSODIUM_INCLUDE_DIRS})
target_link_libraries(trit PRIVATE pthread ZLIB::ZLIB ${LIBSODIUM_LIBRARIES})

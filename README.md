# trit-file-transfer
**trit** is a lightweight CLI tool to transfer files over Wi-Fi or Ethernet using TCP and IP-based communication.

## Build
### Debian/Ubuntu
#### First time setup
1. Install build essentials and cmake
```bash 
sudo apt install build-essential cmake
```

2. Install the asio networking library 
```bash
sudo apt install libasio-dev
```

3. Configure the cmake build system by running the following (in the project root directory)
    - Also run this command whenever CMakeLists.txt is modified (e.g. adding new source files)
```bash
cmake -B build S
```

#### Compilation
1. Compile and build the project executable
```bash
cmake --build build
```

## Usage
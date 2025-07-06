# trit-file-transfer
**trit** is a CLI tool to transfer files over a local network using TCP/IP.

## Build
### Debian/Ubuntu
#### First time setup
1. Install dependencies
```bash 
sudo apt install build-essential cmake libasio-dev zlib1g-dev libsodium-dev pkg-config
```

2. Configure the cmake build system by running the following (in the project root directory)
    - Also run this command whenever CMakeLists.txt is modified (e.g. adding new source files)
```bash
cmake -B build -S .
```

#### Compilation
1. Compile and build the project executable
```bash
cmake --build build
```

## Usage
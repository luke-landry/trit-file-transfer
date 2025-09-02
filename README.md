# trit-file-transfer
**trit** is a CLI tool to transfer files over a local network using TCP/IP.

## Installation
### Ubuntu
1. Build the binary (will be available for download soon)
2. Make the file executable with 
```bash 
chmod +x trit 
```
3. Move it to a directory on your shell's `PATH`,  e.g.:
```bash 
sudo mv trit /usr/local/bin/
```
4. Verify the installation
```bash 
trit help
```

Other Linux distributions and macOS have not been tested. Windows is not currently supported.

## Usage
The basic usage flow is as follows:
1. Sender stages files to send
2. Receiver listens for connections from sender
3. Sender connects to receiver and sends a transfer request
4. Receiver accepts or denies the transfer request
5. Sender begins reading and transferring the staged files if the request is accepted
6. Receiver receives and writes the files

### Commands
Commands follow this syntax: `trit <command> [options]`
| Command                            | Description                                 |
| ---------------------------------- | ------------------------------------------- |
| `trit add <file_pattern>...`       | Stage file(s) for transfer                  |
| `trit drop <file_pattern>...`      | Unstage previously staged file(s)           |
| `trit list`                        | List currently staged files                 |
| `trit send <ip> <port> [password]` | Send a file transfer request to a receiver  |
| `trit receive [password]`          | Start listening for incoming file transfers |
| `trit help`                        | Display help message                        |

### File Pattern Syntax
You can use glob-style patterns when adding or dropping files:
| Pattern         | Matches                                                                           |
| --------------- | --------------------------------------------------------------------------------- |
| `*.ext`         | All files with the given extension in the current directory                       |
| `name.*`        | All files with the given base name and any extension in the current directory     |
| `*`             | All files in the current directory                                                |
| `**`            | All files recursively in the current directory and subdirectories                 |
| `**/*.ext`      | All files with the given extension recursively                                    |
| `path/to/*.ext` | Files with the given extension directly inside `path/to/`                         |
| `path/to/**`    | All files recursively under `path/to/`                                            |
| `?`             | Any single character                                                              |

## Build
### Ubuntu
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

## Implementation
![alt text](images/File_Transfer_Pipeline.png "File Transfer Pipeline Diagram")

### Staging System

Trit uses a staging mechanism so users can queue files before sending.
- Paths to the staging file (`staged.txt`) and a lock directory (`.lock`) are saved under `.trit/` in the current working directory.
- The CLI supports adding, dropping, listing, clearing, sending, and receiving staged files.
- Flexible glob patterns are supported (e.g., `*`, `name.*`, `*.ext`).

### Runtime Metadata & Logging

Trit stores temporary runtime data in two locations:
- Working directory: `.trit/staged.txt` and `.trit/.lock` for staging bookkeeping.
- System temp directory: log file of latest execution at `tmp/trit/log.txt`.

### Networking Layer

Trit uses a lightweight blocking wrapper (`TcpSocket`) built on top of ASIO (standalone, non-Boost).
- Provides cross-platform `connect`, `accept`, `read`, and `write` primitives.
- Simplifies socket management while remaining platform-agnostic.

### Custom Transfer Protocol

#### Handshake & Encryption

- The key is derived from the user-supplied password.
- An authentication tag is appended to each encrypted chunk.
- A mutual-authentication handshake occurs before file metadata exchange:
    1. The sender encrypts a fixed tag with a random nonce, sending the salt, nonce, ciphertext, and stream header.
    2. The receiver derives the key, verifies the tag, and replies with a success byte.

#### Transfer Request

Before data transfer begins, the sender sends a serialized `TransferRequest` containing:
- File count and total transfer size
- Chunk size used in this session
- Per-file metadata (path, size), encoded with length-prefixed strings and fixed-width integers for compactness.

#### Chunk Data
Files are streamed as sequences of fixed-size chunks. Each chunk packet includes:
- 8-byte sequence number
- 1-byte compressed flag
- 2-byte original size
- 2-byte chunk size
- Payload (≤ 65 535 bytes)

### Transfer Pipeline

Trit uses a multi-threaded producer-consumer pipeline for high-throughput transfers.

**Sender:**
- Reads files into buffers.
- Splits large files / aggregates small files into chunks.
- Encrypts chunks and sends over socket.
- Uses bounded queues to decouple producer/consumer stages.
- Tracks progress for throughput monitoring.

**Receiver:**
- Receives chunks from socket.
- Decrypts and verifies integrity.
- Reconstructs files, ensuring directories exist before writing.
- Writes data to disk in parallel via synchronized queues.
- Validates per-file sizes to ensure integrity.

### File I/O Layer

The `FileManager` handles efficient chunking and reconstruction:
- Splits/aggregates files into fixed-size buffers.
- Reassembles files from received chunks.
- Ensures directory structure exists before writes.
- Performs per-file integrity checks against declared sizes.

### Compression (Unutilized)
A compression layer was implemented with zlib to optionally reduce chunk sizes. The compressed flag in each packet was reserved for this purpose.

However, testing showed that throughput decreased with compression:
- CPU cost of compressing/decompressing outweighed bandwidth savings.
- Many files (media, archives, executables) are already compressed.

As a result, compression support remains defined in the protocol but is disabled and not integrated into the transfer pipeline.
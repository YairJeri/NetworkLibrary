# Networking Engine (C++17)

A lightweight, asynchronous multithreaded networking engine built in modern C++17 with Asio (standalone or Boost.Asio). Designed for low-latency client-server architectures, supporting a hybrid transport model: **TCP** for reliable handshakes, session management, and state sync; and **UDP** for rapid, unchoked datagram exchange.

## Features

- **Hybrid Transport Architecture:**
  - **TCP:** Connection lifecycle, framing with variable-length payloads, and disconnect handling.
  - **UDP:** Low-overhead datagrams with an initial authentication handshake associating UDP endpoints to active TCP sessions.
- **Binary Serialization:**
  - Stream operators (`<<`, `>>`) for standard layout POD types and length-prefixed strings (`std::string`).
  - Strict boundary validation against buffer underflow and malformed packets.
- **Thread-Safe Data Structures:**
  - `ThreadSafeQueue`: Condition variable-backed concurrent FIFO queue.
  - `ThreadSafeMap`: Mutex-guarded associative storage for managing client sessions by ID.
- **Asynchronous I/O:** Driven by `asio::io_context` worker threads to avoid blocking caller threads.
- **Zero Heavy Dependencies:** Compatible with standalone `asio` or `boost::asio`.

## Architecture Overview

```
 [ Client ]                                       [ Server ]
     |                                                |
     | ----- 1. TCP Connect (Handshake / Auth) -----> | (Assigns Session ID)
     | <---- 2. TCP Accept + Client ID -------------- |
     |                                                |
     | ----- 3. UDP Handshake (Auth Packet) --------> | (Binds UDP Endpoint to Session)
     |                                                |
     | <================ Hybrid Traffic =============> |
     |      TCP: Reliable Events / Commands           |
     |      UDP: Real-time Datagrams (State/Input)    |
```

## Building and Testing

### Prerequisites
- CMake 3.14+
- C++17 compatible compiler (`g++` 9+, `clang++` 10+)
- `asio` (standalone) or `boost-devel` / `boost`

### Build

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The build produces four targets in `build/`:
- `server`: Example server listening on TCP (port 60000) and UDP (port 60001).
- `client`: Interactive non-blocking terminal client.
- `unit_tests`: Automated test suite for queues, maps, and package serialization.
- `benchmark`: Throughput and latency benchmarking executable.

### Running Unit Tests

```bash
ctest --test-dir build --output-on-failure
# Or directly:
./build/unit_tests
```

### Running Benchmarks

```bash
./build/benchmark
```

**Representative benchmark results (AMD Ryzen / Linux):**
- **Serialization Throughput:** ~29M packets/sec
- **Deserialization Latency:**
  - `p50`: 20 ns
  - `p95`: 22 ns
  - `p99`: 28 ns
- **ThreadSafeQueue Throughput:** ~4.2M operations/sec (4 concurrent producer threads, 1 consumer)

## Project Structure

```
.
├── CMakeLists.txt
├── README.md
├── include/
│   ├── network.h               # Main umbrella header
│   ├── network_core.h          # Core headers, Asio detection, and platform macros
│   ├── network_connection.h    # Asynchronous TCP connection wrapper
│   ├── network_session.h       # Session entity binding TCP connection + UDP endpoint
│   ├── network_package.h       # Binary packet definitions and serialization operators
│   ├── network_server.h        # Server interface (TCP acceptor + UDP listener)
│   ├── network_client.h        # Client interface
│   ├── network_tsqueue.h       # Thread-safe queue
│   └── network_tsmap.h         # Thread-safe map
├── src/
│   ├── server.cpp              # Server example application
│   └── client.cpp              # Client example application
├── tests/
│   └── test_network.cpp        # Unit test suite
└── benchmarks/
    └── benchmark.cpp           # Throughput and latency benchmark tool
```

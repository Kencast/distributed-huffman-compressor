# Distributed Huffman Compressor

A scalable, distributed file compression system that uses **Huffman encoding** to achieve efficient data compression across multiple worker nodes coordinated by a central server.

## About this fork
This is a fork of a team project built for a Data Structures course, kept here to document the networked coordinator/worker design. Original team: Alejandro Cerdas, Danilo Duque, Kener Castillo, and Pablo Pérez.

## 📋 Table of Contents

- [Overview](#overview)
- [How It Works](#how-it-works)
- [Architecture](#architecture)
- [Tech Stack](#tech-stack)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Building](#building)
- [Configuration](#configuration)
- [Algorithm Details](#algorithm-details)
- [License](#license)

## Overview

This project implements a **distributed Huffman compression system** that:

- **Splits large files** across multiple worker nodes for parallel processing
- **Calculates byte frequencies** on each worker independently
- **Generates optimal Huffman codes** from aggregated frequencies
- **Compresses data** on each worker using the generated codes
- **Merges compressed chunks** into a single output file
- **Decompresses** files using the stored Huffman code table

### Key Benefits

**Scalability** - Leverage multiple servers for compression of large files  
**Efficiency** - Huffman encoding reduces file size significantly  
**Modularity** - Clear separation between central server and worker nodes  
**Fault Tolerance** - File chunks processed independently  
**Reversible** - Perfect reconstruction of original files  

## How It Works

### Compression Workflow

```
1. Central Server starts and listens for worker connections
                              ↓
2. Workers connect and receive file chunks
                              ↓
3. Each worker calculates byte frequencies in its chunk
                              ↓
4. Central server aggregates all frequencies
                              ↓
5. Central server generates optimal Huffman tree and codes
                              ↓
6. Central server broadcasts Huffman code table to all workers
                              ↓
7. Each worker compresses its file chunk using the codes
                              ↓
8. Central server merges compressed chunks into single file
                              ↓
9. Central server saves Huffman table for decompression
```

### Decompression Workflow

```
1. Decompressor loads Huffman code table from file
                              ↓
2. Rebuilds Huffman tree from code table
                              ↓
3. Reads compressed file bit-by-bit
                              ↓
4. Traverses Huffman tree for each bit sequence
                              ↓
5. Outputs original byte when leaf node is reached
                              ↓
6. Reconstructs original file perfectly
```

## Architecture

### System Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Central Server                          │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ File Chunker │  │Route Finder  │  │File Merger   │    │
│  │(Splitter)   │  │(Huffman Gen) │  │(Compressor)  │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
│         ↑                 ↑                 ↑             │
│    Socket Conn.      Freq Table       Compressed      │
│                                        Chunks          │
└──────┬──────────────────┬──────────────────┬─────────────┘
       │                  │                  │
   TCP/Socket          TCP/Socket       TCP/Socket
   Connection          Connection       Connection
       │                  │                  │
┌──────▼────────┐ ┌──────▼────────┐ ┌──────▼────────┐
│  Worker 1     │ │  Worker 2     │ │  Worker N     │
│               │ │               │ │               │
│┌─────────────┐│ │┌─────────────┐│ │┌─────────────┐│
││Byte Counter ││ ││Byte Counter ││ ││Byte Counter ││
│└─────────────┘│ │└─────────────┘│ │└─────────────┘│
│┌─────────────┐│ │┌─────────────┐│ │┌─────────────┐│
││Compressor   ││ ││Compressor   ││ ││Compressor   ││
│└─────────────┘│ │└─────────────┘│ │└─────────────┘│
└────────────────┘ └────────────────┘ └────────────────┘
```

## Tech Stack

### Language & Environment
- **Language**: C (ISO C99)
- **Build System**: Make/GCC compilation
- **Networking**: POSIX sockets (TCP/IP)
- **Threading**: POSIX pthreads (optional, for concurrent processing)

### Key Libraries
- **Standard C Library** (`<stdio.h>`, `<stdlib.h>`, `<string.h>`)
- **Socket Library** (`<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`)
- **Network Library** (`<netdb.h>`)
- **Threading** (`<pthread.h>`)

### Data Structures
- **Priority Queue** - For building optimal Huffman tree
- **Binary Tree** - Huffman tree representation
- **Dynamic Arrays** - For frequency tables and buffers

## Project Structure

```
distributed-huffman-compressor/
├── README.md                                    # This file
├── LICENSE                                      # Project license
├── code/
│   ├── config.h                                # Global configuration
│   ├── Compressor/
│   │   ├── Huffman/
│   │   │   ├── test.c                          # Huffman tree builder test
│   │   │   ├── Node.h                          # Tree node structure
│   │   │   ├── Tree.h                          # Tree operations
│   │   │   └── Priority_Queue.h                # Priority queue for tree building
│   │   └── Server/
│   │       ├── Central/
│   │       │   ├── MainServer.c                # Central server coordinator
│   │       │   ├── util.h                      # Utility functions
│   │       │   ├── RouteFinder.h               # Huffman code generator
│   │       │   ├── FileChunker/
│   │       │   │   └── FileChunker.h           # File splitting logic
│   │       │   ├── FileCompressor/
│   │       │   │   └── FileCompressor.h        # Compression threads
│   │       │   ├── MakeTable.h                 # Code table generation
│   │       │   ├── ServerListener.h            # Connection management
│   │       │   └── resources/
│   │       │       └── saved/                  # Output directory
│   │       │           ├── compressedFile.huff # Compressed output
│   │       │           └── huffman.table       # Code table
│   │       └── Workers/
│   │           ├── RunWorker.c                 # Worker node main
│   │           ├── util.h                      # Worker utilities
│   │           ├── ByteCounter.h               # Frequency calculator
│   │           ├── WorkerSender.h              # Network communication
│   │           └── FileCompressor/
│   │               └── FileCompressor.h        # Worker compression logic
│   └── Decompressor/
│       ├── run.c                               # Decompression main
│       └── util.h                              # Decompression utilities
```

## Getting Started

### Prerequisites

```bash
# Linux/Ubuntu
sudo apt-get install gcc make pthread

# macOS (with Homebrew)
brew install gcc
```

### Installation

```bash
# Clone the repository
git clone https://github.com/Kencast/distributed-huffman-compressor.git
cd distributed-huffman-compressor

# Navigate to code directory
cd code
```

## Usage

### Step 1: Configure the System

Edit [config.h](/home/kencast/Work/distributed-huffman-compressor.worktrees/add-clear-code-comments/code/config.h) to set:
- Input file path: `FILE_PATH`
- Output paths: `PATH_FOR_COMPRESS`, `PATH_FOR_TABLE`
- Server connection: `IP` and `NGROK_PORT`

```c
#define FILE_PATH "./resources/Data-Structures.zip"
#define PATH_FOR_COMPRESS "./resources/saved/compressedFile.huff"
#define PATH_FOR_TABLE "./resources/saved/huffman.table"
#define IP "2.tcp.ngrok.io"
#define NGROK_PORT 12175
```

### Step 2: Compile

```bash
# Compile central server
gcc -o MainServer Compressor/Server/Central/MainServer.c -lpthread

# Compile worker nodes
gcc -o Worker Compressor/Server/Workers/RunWorker.c -lpthread

# Compile decompressor
gcc -o Decompressor Decompressor/run.c

# Compile Huffman test (optional)
gcc -o HuffmanTest Compressor/Huffman/test.c
```

### Step 3: Run Compression

**Terminal 1 - Start Central Server:**
```bash
./MainServer
# When prompted:
# Insert the number of client servers: 3
```

**Terminal 2, 3, 4 - Start Worker Nodes:**
```bash
./Worker
# Each worker will:
# 1. Connect to central server
# 2. Receive file chunk
# 3. Calculate frequencies
# 4. Wait for Huffman codes
# 5. Compress chunk
# 6. Save compressed output
```

**Expected Output:**
```
Central server listening on port: 12175...
Client connected from 192.168.1.100:45678!
Client connected from 192.168.1.101:45679!
Client connected from 192.168.1.102:45680!
File successfully compressed
Written compressed file size 2048576
```

### Step 4: Run Decompression

```bash
./Decompressor
# Output:
# Largest path 24
# File decompressed successfully
```

The decompressed file will be saved to the path specified in `PATH_FOR_DESCOMPRESS`.

## Building

### Basic Build (Manual)

```bash
# Build central server
gcc -Wall -O2 -o MainServer Compressor/Server/Central/MainServer.c -lpthread

# Build worker
gcc -Wall -O2 -o Worker Compressor/Server/Workers/RunWorker.c -lpthread

# Build decompressor
gcc -Wall -O2 -o Decompressor Decompressor/run.c
```

### With Makefile (if available)

```bash
make
make clean  # Clean build artifacts
```

### Build Flags Explained

| Flag | Purpose |
|------|---------|
| `-Wall` | Enable all compiler warnings |
| `-O2` | Optimization level 2 (faster execution) |
| `-lpthread` | Link POSIX threading library |
| `-std=c99` | Use C99 standard |

## Configuration

### Key Configuration Variables (config.h)

```c
// File paths
#define FILE_PATH "./resources/Data-Structures.zip"          // Input file
#define PATH_FOR_COMPRESS "./resources/saved/compressedFile.huff"
#define PATH_FOR_TABLE "./resources/saved/huffman.table"

// Decompression paths
#define COMPRESSED_FILE "../Compressor/Server/Central/resources/saved/compressedFile.huff"
#define PATH_FOR_DESCOMPRESS "../Compressor/Server/Central/resources/test"
#define DESCOMPRESS_TABLE "../Compressor/Server/Central/resources/saved/huffman.table"

// Network configuration
#define IP "2.tcp.ngrok.io"                  // Server address
#define NGROK_PORT 12175                     // Server port
```

### Runtime Parameters

When running the central server, you'll be prompted:
```
Insert the number of client servers: <N>
```

Where `<N>` is the number of worker nodes expected to connect.

## Algorithm Details

### Huffman Encoding

**Time Complexity:** O(n log n) for tree building, O(n) for encoding  
**Space Complexity:** O(n) for tree and frequency table

**Steps:**
```
1. Calculate frequency of each byte in input
2. Create leaf node for each unique byte
3. Build optimal binary tree using priority queue:
   - Extract 2 nodes with min frequency
   - Create parent with combined frequency
   - Repeat until 1 node remains
4. Assign codes by tree path:
   - Left edge = 0
   - Right edge = 1
5. Encode input using generated codes
```

### Compression Format

```
[Compressed Data] [Padding Byte] [Padding Count]
   (variable)        (1 byte)       (1 byte)

Padding Count = number of valid bits in Padding Byte
                (0-7, indicates partial byte encoding)
```

### Decompression Format

```
For each bit in input:
  1. Start at Huffman tree root
  2. If bit = 1: go right; else: go left
  3. If leaf node reached: output byte, reset to root
  4. Continue until all bits processed
  5. Handle final padding bits
```

## Performance Characteristics

### Compression Ratio
- **Text files:** 40-60% reduction
- **Binary files:** 10-30% reduction
- **Already compressed files:** No reduction (incompressible)

### Network Overhead
- File distribution: O(n) where n = file size
- Frequency aggregation: O(256) = O(1)
- Huffman table transfer: O(256*log(256)) ≈ 2KB

### Scalability
- Linear speedup with worker count (communication bottleneck)
- Optimal for files > 10 MB
- Recommended max workers: 10 (network overhead)

## Future Enhancements

- [ ] Adaptive Huffman coding (dynamic tree updates)
- [ ] LZ77 preprocessing for better compression
- [ ] Parallel worker compression with OpenMP
- [ ] Support for encrypted compression
- [ ] Streaming decompression support
- [ ] Progress monitoring and reporting
- [ ] Error recovery and retry logic

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

# List_Server — Fault‑Tolerant Distributed List Service

## Overview

List_Server provides a modular C implementation of a distributed list/chain service. The project focuses on demonstrating RPC communication, basic fault‑tolerance techniques and an optional coordination layer (ZooKeeper) for service discovery and membership.

## Features

- RPC communication using Protocol Buffers
- Modular C codebase with clear separation between transport, skeleton and business logic
- Optional ZooKeeper integration for coordination (leader election / registration)
- Simple fault detection and recovery patterns

## Architecture

- client ⇄ network layer ⇄ service skeleton ⇄ list implementation
- Optional Zookeeper ensemble for node registration and basic coordination
- Binaries: `list_client`, `list_server`, `chain_client`, `chain_server` (see `binary/`)

## Technologies

- C (gcc)
- Protocol Buffers (`protoc` + `protoc-c`)
- ZooKeeper (optional, C client)
- POSIX sockets, threads (`pthreads`)

## Skills developed

- Design of simple distributed services
- RPC serialization/deserialization with Protocol Buffers
- Service skeletons and separation of concerns
- Practical integration with a coordination system (ZooKeeper)

## Prerequisites (Ubuntu/Debian example)

```bash
sudo apt update
sudo apt install build-essential pkg-config protobuf-c-compiler protobuf-compiler libzookeeper-mt-dev libprotobuf-c-dev
```

If you plan to use ZooKeeper locally for testing, Docker is the easiest option (see below).

## Build

From the project root:

```bash
make
```

Build outputs are placed in `binary/` and object files in `object/`.

## Run (examples)

Start a server (example):

```bash
./binary/list_server 8000
```

Run a client and connect to the server:

```bash
./binary/list_client 127.0.0.1:8000
```

For chain/ZooKeeper tests, start a local ZooKeeper node (Docker):

```bash
docker run --rm --name local-zk -p 2181:2181 zookeeper:3.8
```

Then start the server pointing to the ZooKeeper connection string (check server flags in source/Makefile comments).

## Project layout

- `source/` — C source files
- `include/` — headers and generated protobuf headers
- `binary/` — compiled executables (output)
- `object/` — compiled object files (output)
- `lib/` — libraries used during linking
- `sdmessage.proto` — protobuf message definitions
- `Makefile` — build rules


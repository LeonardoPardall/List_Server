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

<<<<<<< HEAD
=======
## Architecture & Modules

This project implements a small distributed list/chain service. Main components:

- `chain_server` — server process that exposes the list/chain service. Handles requests, updates local state and (optionally) coordinates with peers.
- `chain_client` — client program that connects to a server and issues list/chain operations.
- `network_server` / `network_client` — transport/RPC adapters implementing message send/receive and (de)serialization using Protocol Buffers defined in `sdmessage.proto`.
- `list_skel`, `list_*` — service skeleton and core list implementation (business logic and state transitions).

The common flow is: `chain_client` → RPC (protobuf message) → `chain_server` → service skeleton → update state / reply. See the `source/` files for entry points and parameter parsing.

## Communication & Coordination

The project uses protobuf-based RPC for client-server communication (see `sdmessage.proto` and the `network_client` / `network_server` modules). Messages are serialized with Protocol Buffers and exchanged via the RPC layer implemented in the codebase.

Zookeeper is supported for deployments that require service discovery or leader election, but the core of the project is the RPC/service implementation — see `Architecture & Modules` above for details on how pieces interact.

## Zookeeper integration

This project is designed to integrate with Apache Zookeeper for service discovery and coordination between server instances. The server components can be configured to register themselves with a Zookeeper ensemble and use Zookeeper primitives (znodes, watches, ephemeral nodes) for leader election or cluster membership.

Quick ways to run a local Zookeeper for development:

- Docker (fast, recommended for local testing):

```bash
# run a single-node Zookeeper on localhost:2181
docker run --rm --name local-zk -p 2181:2181 zookeeper:3.8
```

- Native install: follow Apache Zookeeper documentation and start a standalone server listening on `localhost:2181`.

Configuration notes

- The code expects a Zookeeper connection string (e.g. `localhost:2181` or `zk1:2181,zk2:2181`) to be provided to the server. Check the server startup parameters or configuration file (search for `zookeeper`, `zk_connect` or similar in the source) and update it with your ensemble address.


Testing the integration

- Start Zookeeper (using Docker as above).
- Launch one or more server instances (pointing them to the Zookeeper address).
- Confirm the servers register in Zookeeper (use `zkCli.sh` or a ZK client to list znodes), or check server logs for successful registration and leader election messages.


## Important notes

- The project was implemented with the behaviour observed from provided binaries (as instructed). Some details were adapted to ensure compatibility.
- The implementation validates vehicle year values to be in a sensible range (1886 → current year + 1).
- `server.log` is created in the working directory when the server runs.


>>>>>>> 74a85bfb6e769a9ee8d5067dfb8372d356ccea54

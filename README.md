# SD-07 — Project 2: SOCHAIN

**Authors**

- Afonso Henriques — 61826
- Leonardo Pardal — 61836
- Pedro Carvalho — 61800

## Overview

The project implements a simple distributed list/chain service with client and server components written in C. The source code, headers, protobuf definitions and Makefile are included so the project can be built and inspected.

## Prerequisites

- A C toolchain (GCC/Clang)
- `make` (GNU Make)
- `protoc` and `protoc-c` if you need to regenerate protobuf C sources
- Standard build tools (e.g. `build-essential` on Debian/Ubuntu)

Example (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install build-essential protobuf-c-compiler protobuf-compiler pkg-config
```

## Build

From the repository root run:

```bash
make
```

This will compile the project and place the executables in the `binary/` directory. Use `make clean` to remove build artifacts.

## Run (example)

The compiled executables are available under `binary/`. Example usage (replace placeholders with your environment values):

```bash
# start server (example)
./binary/list_server <port>

# run client and connect to server
./binary/list_client <server_ip>:<port>
```

Check the `binary/` folder or the `Makefile` outputs for the exact binary names and parameters.

## Project layout

- `source/` — C source files
- `include/` — headers and generated protobuf headers
- `binary/` — compiled executables (output)
- `object/` — compiled object files (output)
- `lib/` — libraries used during linking
- `sdmessage.proto` — protobuf message definitions
- `Makefile` — build rules

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



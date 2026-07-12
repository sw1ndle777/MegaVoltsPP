# MegaVoltsPP

A fully reverse-engineered private game server for **MicroVolts** (Plaza version 1.0.0 through 1.0.3, circa 2013). Built from scratch in modern C++23 after years of binary analysis using IDA Pro, this is the only private server implementation that has successfully replicated the original game's network protocol, encryption, and gameplay systems.

The server handles the full game lifecycle: authentication, lobby management, room creation, real-time match gameplay, item economy, clans, social systems, chat, mail, battle passes, and more. It communicates with the unmodified retail client as a drop-in replacement for the original backend.

There is also optional cross-version support for older and newer client builds, though those paths are deprecated in favor of the 1.0.3 target.

## Architecture

MegaVoltsPP runs as three cooperating server processes that communicate over a persistent IPC layer:

| Process | Role |
|---------|------|
| **Front** | Login gateway. Handles client authentication (username/password or reconnect key), encryption handshake, and routes players to Main. |
| **Main** | Lobby and game logic. Manages channels, rooms, parties, matchmaking, inventory, item shop, gacha, clans, friends, mail, chat, battle pass, daily/weekly/monthly rewards, moderation, and all pre/post-match state. |
| **Cast** | Real-time match relay. Handles in-match gameplay: player movement batching, weapon attacks (rifle, shotgun, sniper, gatling, melee, projectile), NPC spawning, item pickups/kit drops, bomb mode, respawns, tick synchronization, and host-authoritative combat. |

### Networking

- **Async I/O**: All networking is built on [Asio](https://think-async.com/Asio/) (standalone, non-Boost) with `io_context` and strands for lock-free concurrent dispatch.
- **Multithreaded**: Each server runs a configurable pool of `std::jthread` worker threads on the `io_context`, with a watchdog that detects stalled threads.
- **Protocol**: Custom binary TCP protocol with an 8-byte packet header. Packets are encrypted using RC5/RC6 ciphers with per-session serial keys, matching the original client implementation.
- **IPC**: Inter-process communication between Front/Main/Cast uses persistent TCP connections with automatic reconnection and binary message framing.
- **Rate limiting**: Per-packet rate limiting with configurable rules, cooldowns, strike tracking, and identity-scoped blacklisting.

### Database

Dual database backend with a shared `IDatabase` interface:

- **MariaDB** via mariadb-connector-cpp
- **PostgreSQL** via libpqxx

All database writes go through a transactional batch pipeline that validates changes in cache before persisting, with automatic reconnection on connection loss. Database work is dispatched to a dedicated thread pool to keep the I/O threads responsive.

### Security & Anticheat

- **Encryption**: Full RC5 (32/64-bit block) and RC6 (128-bit block) cipher implementation with key expansion, matching the original client's cryptography byte-for-byte — plus custom modifications and extensions to the cipher dispatch, key scheduling, and error handling beyond what the original client shipped with.
- **Header encryption**: Custom TCP header obfuscation layer with bit rotation, nibble swaps, XOR masking, and additional tampering not present in the original protocol.
- **Password hashing**: Argon2-based password hashing via Monocypher with per-account salts.
- **MegaGuard**: Communication protocol for the companion anticheat project (a separate DLL injected into the client) with heartbeat verification.
- **Packet validation**: Bounds checking, size validation, and malformed packet rejection to prevent crashes from crafted input.

### Monitoring

- **BetterStack heartbeat**: HTTPS heartbeat pings gated on `io_context` liveness. If the server crashes or hangs, heartbeats stop and the monitor flips to down.
- **Crash reporting**: Integrated Crashpad for crash dump collection and automatic `/fail` heartbeat on `std::terminate`.
- **Watchdog**: Thread-level watchdog that detects and logs stalled handler executions.

## Dependencies

Managed via [vcpkg](https://vcpkg.io/) (auto-fetched by CMake):

| Library | Purpose |
|---------|---------|
| [Asio](https://think-async.com/Asio/) | Async networking and I/O |
| [fmt](https://fmt.dev/) | Text formatting and colored console output |
| [Crashpad](https://chromium.googlesource.com/crashpad/crashpad/) | Crash dump collection |
| [MariaDB Connector/C++](https://mariadb.com/kb/en/mariadb-connector-cpp/) | MariaDB database driver |
| [libpqxx](https://pqxx.org/libpqxx/) | PostgreSQL database driver |
| [OpenSSL](https://www.openssl.org/) | TLS for heartbeat HTTPS and secure connections |
| [libsodium](https://libsodium.gitbook.io/) | Cryptographic primitives |
| [RapidJSON](https://rapidjson.org/) | JSON configuration parsing |
| [Monocypher](https://monocypher.org/) | Password hashing (Argon2) |
| [magic_enum](https://github.com/Neargye/magic_enum) | Enum reflection |
| [Boost.Unordered](https://www.boost.org/doc/libs/release/libs/unordered/) | `flat_map` / `flat_set` for performance-critical lookups |
| [thread-pool](https://github.com/bshoshany/thread-pool) | Database thread pool |
| [Google Test](https://github.com/google/googletest) | Unit testing |

## Building

### Requirements

- **CMake** 3.30+ with [cmkr](https://cmkr.build/) (`cmake.toml` generates `CMakeLists.txt`)
- A **C++23**-capable compiler (MSVC, Clang, or GCC)
- **Ninja** (recommended) or any CMake generator
- Git (for vcpkg auto-fetch)

vcpkg is automatically downloaded and bootstrapped by CMake on the first configure. No manual vcpkg installation needed.

### Windows (MSVC)

```bash
# From a Visual Studio Developer Command Prompt (vcvars64)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target front main cast
```

### Windows (Clang)

```bash
# Requires LLVM/Clang 19+ installed
cmake -S . -B build -G Ninja ^
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ^
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target front main cast
```

### Windows (clang-cl)

```bash
# From a Visual Studio Developer Command Prompt
cmake -S . -B build -G Ninja ^
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl ^
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target front main cast
```

### Linux (Clang)

```bash
sudo apt install clang-20 ninja-build
CC=clang-20 CXX=clang++-20 cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target front main cast
```

### Linux (GCC)

```bash
sudo apt install gcc-15 g++-15 ninja-build
CC=gcc-15 CXX=g++-15 cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target front main cast
```

Binaries are output to `out/<platform>-<compiler>/bin/Release/`.

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `MVPP_MARCH_NATIVE` | `ON` | Use `-march=native` for local builds. Set to `OFF` for portable/Docker builds (falls back to `x86-64-v2`). |

The build system also auto-detects and uses `ccache` or `sccache` if available.

### Running Tests

```bash
cmake --build build --target unit_tests
ctest --test-dir build --output-on-failure
```

Tests are built with [Google Test](https://github.com/google/googletest) and cover:

- **Crypto**: Auth key generation, BLAKE2b hashing, Argon2 password hashing
- **Database**: Error handling, driver factory, SQL placeholder generation
- **Game logic**: Boss rewards, currency validation, gacha pity, item upgrades, weighted selection, serial allocation, package items
- **Room**: Team balancing, votekick rules
- **Utility**: Base64 encode/decode, nickname validation, duration parsing, number parsing, safe string reads, URL parsing, time utilities
- **Stress**: Connection stress testing

## Docker

### Build the image

```bash
docker build -f docker/Dockerfile.server -t megavoltspp-server:clang19 .
```

The Dockerfile uses a multi-stage build: Clang 19 on Ubuntu 24.04 for compilation, then a minimal runtime image with just the binaries and shared libraries.

### Run with Docker Compose

```bash
# Start all three servers (set DB credentials via environment or .env file)
docker compose up -d

# Or start with a local MariaDB instance
docker compose --profile local-db up -d

# Or start with a local PostgreSQL instance
docker compose --profile local-pg up -d
```

The compose file starts three containers (front, main, cast) on a shared bridge network with static IPs. Each server's ports, IPC ports, and database credentials are configurable through environment variables.

#### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `MVPP_DB_DRIVER` | `mariadb` | Database driver (`mariadb` or `postgresql`) |
| `MVPP_DB_HOST` | `host.docker.internal` | Database host |
| `MVPP_DB_PORT` | `3306` | Database port |
| `MVPP_DB_NAME` | `megavoltspp` | Database name |
| `MVPP_DB_USER` | `root` | Database username |
| `MVPP_DB_PASSWORD` | *(required)* | Database password |
| `MVPP_FRONT_PORT` | `13000` | Front server client port |
| `MVPP_MAIN_PORT` | `13005` | Main server port |
| `MVPP_CAST_PORT` | `13006` | Cast server port |

See `docker-compose.yml` for the full list of configurable ports and network settings.

## Configuration

The server reads `settings.json` at startup. This file contains server bind addresses, ports, database credentials, and feature flags. It is excluded from version control (`.gitignore`) since it contains sensitive information.

A settings file is generated automatically by the Docker entrypoint script (`docker/configure_settings.py`) from environment variables when running in containers.

For local development, create a `settings.json` in the working directory with your database connection details and server addresses.



# MegaVoltsPP Docker setup (main/front/cast)

This setup builds your C++23 servers with clang (LLVM 19) and runs all three services via Docker Compose.

## 1) Place runtime data on host

Expected host structure:

```text
out/
  bin/
    settings.json
    file_integrity.json
  cgd/
    ...
```

Compose mounts `./out` as read-only `/seed` and the entrypoint copies:

- `cgd` from `/seed/cgd` to `/app/cgd`
- `settings.json` from `/seed/bin/settings.json` to `/app/bin/settings.json`
- `file_integrity.json` from `/seed/bin/file_integrity.json` to `/app/bin/file_integrity.json`

If files are missing, safe fallbacks are used.

## 2) External DB on Windows host (default)

By default, DB host is `host.docker.internal`.

Set DB credentials/environment before starting:

```powershell
$env:MVPP_DB_HOST = "host.docker.internal"
$env:MVPP_DB_PORT = "3306"
$env:MVPP_DB_NAME = "your_db"
$env:MVPP_DB_USER = "your_user"
$env:MVPP_DB_PASSWORD = "your_password"
```

### 2.1) TLS / self-signed certificate options

For Windows-host MariaDB/MySQL with self-signed TLS, place certificates in:

```text
docker-data/
  certs/
    db-ca.pem
    # optional client cert/key if your server requires mTLS:
    # client-cert.pem
    # client-key.pem
```

Compose mounts `./docker-data/certs` into `/app/certs` inside containers.

Common options:

```powershell
# Preferred: verify certificate chain with provided CA
$env:MVPP_DB_SSL_MODE = "verify-ca"
$env:MVPP_DB_USE_TLS = "true"
$env:MVPP_DB_TLS_CA = "/app/certs/db-ca.pem"

# Optional when server cert CN/SAN doesn't match host.docker.internal
$env:MVPP_DB_DISABLE_SSL_HOSTNAME_VERIFICATION = "true"

# Alternative (less strict): trust mode, no cert validation
# $env:MVPP_DB_SSL_MODE = "trust"
# $env:MVPP_DB_TRUST_SERVER_CERTIFICATE = "true"

# Optional extra TLS inputs (only if needed by your server policy)
# $env:MVPP_DB_SERVER_SSL_CERT = "/app/certs/db-ca.pem"
# $env:MVPP_DB_TLS_CERT = "/app/certs/client-cert.pem"
# $env:MVPP_DB_TLS_KEY = "/app/certs/client-key.pem"
# $env:MVPP_DB_TLS_VERSION = "TLSv1.2"
```

If you keep `MVPP_DB_SSL_MODE=disable`, the server can still require TLS depending on host-side auth policy.

## 2.2) Static service IPs (needed by current IPC whitelist logic)

Current server code whitelists IPC addresses from `settings.json` host IPs, and front also binds using `servers.main.host`.

Compose is configured with static IPs by default:

- main: `172.30.0.10`
- front: `172.30.0.11`
- cast: `172.30.0.12`

You can override via env vars:

```powershell
$env:MVPP_DOCKER_SUBNET = "172.30.0.0/24"
$env:MVPP_MAIN_HOST = "172.30.0.10"
$env:MVPP_FRONT_HOST = "172.30.0.11"
$env:MVPP_CAST_HOST = "172.30.0.12"
```

If that subnet conflicts with another Docker network on your machine, set a different one before `docker compose up`.

## 3) Build and run all 3 servers

```powershell
docker compose up --build -d main front cast
```

## 4) Optional local MariaDB in compose

If you want DB in compose instead of Windows-host DB:

```powershell
$env:MVPP_DB_HOST = "mariadb"
docker compose --profile local-db up --build -d
```

## 5) Crashpad on Linux

Linux handler name is `crashpad_handler` (no `.exe`).

Entrypoint looks for it in:

- `/seed/crash_dumps/crashpad_handler`
- `/seed/bin/crashpad_handler`
- bundled build output fallback

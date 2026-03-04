#!/usr/bin/env python3
import json
import os
import pathlib
import sys


def env_int(name: str, default: int) -> int:
    raw = os.getenv(name)
    if raw is None or raw.strip() == "":
        return default
    try:
        return int(raw)
    except ValueError:
        return default


def env_bool(name: str, default: bool) -> bool:
    raw = os.getenv(name)
    if raw is None or raw.strip() == "":
        return default
    return raw.strip().lower() in {"1", "true", "yes", "on"}


def ensure_host_block(servers: dict, name: str, host: str, port: int, ipc_port: int) -> None:
    block = servers.setdefault(name, {})
    block.setdefault("debug", False)
    block.setdefault("watchguard", False)
    block.setdefault("gacha_pity_enabled", True)
    block.setdefault("asio_threads", 0)
    block.setdefault("database_threads", 0)
    block.setdefault("logger_threads", 0)
    block.setdefault("playtime_min_seconds", 0)
    block["host"] = host
    block["port"] = port
    block["ipc_port"] = ipc_port


def main() -> int:
    settings_path = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "/app/bin/settings.json")

    data = {}
    if settings_path.exists():
        try:
            data = json.loads(settings_path.read_text(encoding="utf-8"))
        except Exception:
            data = {}

    servers = data.setdefault("servers", {})

    ensure_host_block(
        servers,
        "front",
        os.getenv("FRONT_HOST", "0.0.0.0"),
        env_int("FRONT_PORT", 13000),
        env_int("FRONT_IPC_PORT", 12000),
    )
    ensure_host_block(
        servers,
        "main",
        os.getenv("MAIN_HOST", "0.0.0.0"),
        env_int("MAIN_PORT", 13005),
        env_int("MAIN_IPC_PORT", 12005),
    )
    ensure_host_block(
        servers,
        "cast",
        os.getenv("CAST_HOST", "0.0.0.0"),
        env_int("CAST_PORT", 13006),
        env_int("CAST_IPC_PORT", 12006),
    )

    # Optional boolean overrides
    servers["front"]["debug"] = env_bool("FRONT_DEBUG", bool(servers["front"].get("debug", False)))
    servers["main"]["debug"] = env_bool("MAIN_DEBUG", bool(servers["main"].get("debug", False)))
    servers["cast"]["debug"] = env_bool("CAST_DEBUG", bool(servers["cast"].get("debug", False)))
    servers["front"]["watchguard"] = env_bool("FRONT_WATCHGUARD", bool(servers["front"].get("watchguard", False)))
    servers["main"]["watchguard"] = env_bool("MAIN_WATCHGUARD", bool(servers["main"].get("watchguard", False)))
    servers["cast"]["watchguard"] = env_bool("CAST_WATCHGUARD", bool(servers["cast"].get("watchguard", False)))

    db = servers.setdefault("database", {})
    db["host"] = os.getenv("DB_HOST", "host.docker.internal")
    db["port"] = env_int("DB_PORT", 3306)
    db["db_name"] = os.getenv("DB_NAME", db.get("db_name", ""))
    db["user"] = os.getenv("DB_USER", db.get("user", ""))
    db["password"] = os.getenv("DB_PASSWORD", db.get("password", ""))

    # MariaDB connector properties (optional)
    # Helps when host-side MySQL/MariaDB is configured with self-signed TLS.
    db["sslMode"] = os.getenv("DB_SSL_MODE", db.get("sslMode", "disable"))
    db["tlsVersion"] = os.getenv("DB_TLS_VERSION", db.get("tlsVersion", ""))
    db["serverSslCert"] = os.getenv("DB_SERVER_SSL_CERT", db.get("serverSslCert", ""))

    website = servers.setdefault("website", {})
    website["host"] = os.getenv("WEBSITE_HOST", str(website.get("host", "127.0.0.1")))
    website["port"] = env_int("WEBSITE_PORT", int(website.get("port", 80)))
    website["timeout"] = env_int("WEBSITE_TIMEOUT", int(website.get("timeout", 2000)))

    settings_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

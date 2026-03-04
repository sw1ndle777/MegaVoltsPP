#!/usr/bin/env bash
set -euo pipefail

SERVICE_TARGET="${SERVICE_TARGET:-main}"

copy_first_existing() {
  local destination="$1"
  shift

  local source
  for source in "$@"; do
    if [[ -f "${source}" ]]; then
      cp -f "${source}" "${destination}"
      return 0
    fi
  done
  return 1
}

copy_first_existing_dir() {
  local destination="$1"
  shift

  local source
  for source in "$@"; do
    if [[ -d "${source}" ]]; then
      rm -rf "${destination}"
      mkdir -p "${destination}"
      cp -a "${source}"/. "${destination}"/
      return 0
    fi
  done
  return 1
}

mkdir -p /app/bin /app/cgd /app/logs /app/crash_dumps /app/vcpkg-lib /app/vcpkg-bin

# Optional DB cert material mount for TLS validation/trust mode
mkdir -p /app/certs
if [[ -d /seed/certs ]]; then
  cp -a /seed/certs/. /app/certs/ 2>/dev/null || true
fi

# Refresh binaries from bundled build every start (ensures the selected image version is used)
cp -f /opt/mvpp/bin/front /app/bin/front
cp -f /opt/mvpp/bin/main /app/bin/main
cp -f /opt/mvpp/bin/cast /app/bin/cast
chmod +x /app/bin/front /app/bin/main /app/bin/cast

# Runtime libraries from vcpkg dynamic packages
if [[ -d /opt/mvpp/vcpkg-lib ]]; then
  cp -a /opt/mvpp/vcpkg-lib/. /app/vcpkg-lib/ 2>/dev/null || true
fi
if [[ -d /opt/mvpp/vcpkg-bin ]]; then
  cp -a /opt/mvpp/vcpkg-bin/. /app/vcpkg-bin/ 2>/dev/null || true
fi

# cgd from mounted /seed first, then bundled fallback
# Support both:
#  - out/cgd/*
#  - out/bin/cgd/*
#  - out/linux-clang/bin/Release/cgd/*
copy_first_existing_dir /app/cgd \
  /seed/cgd \
  /seed/bin/cgd \
  /seed/linux-clang/bin/Release/cgd \
  /opt/mvpp/cgd \
  || true

# Linux filesystem is case-sensitive. Some shipped data may have mixed-case names
# while server code expects lowercase names from Windows-era paths.
if [[ ! -f /app/cgd/roomoptioninfo.cdb && -f /app/cgd/roomoptionInfo.cdb ]]; then
  cp -f /app/cgd/roomoptionInfo.cdb /app/cgd/roomoptioninfo.cdb
fi

# settings.json + file_integrity.json from out/bin (or out/linux-clang/bin/Release)
copy_first_existing /app/bin/settings.json \
  /seed/bin/settings.json \
  /seed/linux-clang/bin/Release/settings.json \
  /opt/mvpp/bin/settings.json \
  || true

copy_first_existing /app/bin/file_integrity.json \
  /seed/bin/file_integrity.json \
  /seed/linux-clang/bin/Release/file_integrity.json \
  /opt/mvpp/bin/file_integrity.json \
  || true

# crashpad handler on Linux is "crashpad_handler" (no .exe)
copy_first_existing /app/crash_dumps/crashpad_handler \
  /seed/crash_dumps/crashpad_handler \
  /seed/bin/crashpad_handler \
  /opt/mvpp/crash_dumps/crashpad_handler \
  || true
chmod +x /app/crash_dumps/crashpad_handler 2>/dev/null || true

if [[ ! -f /app/bin/settings.json ]]; then
  printf "{}\n" > /app/bin/settings.json
fi

python3 /usr/local/bin/configure_settings.py /app/bin/settings.json

case "${SERVICE_TARGET}" in
  main|front|cast)
    ;;
  *)
    echo "Unsupported SERVICE_TARGET='${SERVICE_TARGET}'. Expected: main|front|cast" >&2
    exit 1
    ;;
esac

cd /app/bin
exec "/app/bin/${SERVICE_TARGET}"

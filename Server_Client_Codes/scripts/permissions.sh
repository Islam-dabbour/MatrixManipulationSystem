#!/bin/bash
#
# permissions.sh
# Manages file permissions for the matrix-processing system: makes compiled
# binaries executable, restricts log files to prevent accidental tampering,
# and normalizes FIFO permissions so worker processes can always open them.
#
# Usage:
#   ./permissions.sh apply  <src_dir>     # set correct perms after build
#   ./permissions.sh lock   <log_dir>     # lock down logs once a run ends
#   ./permissions.sh audit  <target_dir>  # report any files with unsafe perms

set -euo pipefail

ACTION="${1:-}"
TARGET="${2:-.}"

apply_permissions() {
    local dir="$1"
    echo "[PERMISSIONS] Applying executable permissions in: ${dir}"

    find "${dir}" -maxdepth 1 -type f \
        \( -name "server" -o -name "client" -o -name "*_worker" \
           -o -name "logger_process" -o -name "time_process" \) \
        -exec chmod 755 {} \;

    # FIFOs must be readable/writable by the owning user and group only
    find "${dir}" -maxdepth 1 -type p -exec chmod 660 {} \; 2>/dev/null || true

    echo "[PERMISSIONS] Done."
}

lock_logs() {
    local dir="$1"
    echo "[PERMISSIONS] Locking log files in: ${dir}"

    find "${dir}" -type f -name "*.log" -exec chmod 440 {} \;

    echo "[PERMISSIONS] Logs are now read-only."
}

audit_permissions() {
    local dir="$1"
    echo "[PERMISSIONS] Auditing world-writable files under: ${dir}"

    find "${dir}" -type f -perm -o+w -print | while read -r file; do
        echo "  [WARN] World-writable: ${file}"
    done

    echo "[PERMISSIONS] Audit complete."
}

case "${ACTION}" in
    apply) apply_permissions "${TARGET}" ;;
    lock)  lock_logs "${TARGET}" ;;
    audit) audit_permissions "${TARGET}" ;;
    *)
        echo "Usage: $0 {apply|lock|audit} <directory>"
        exit 1
        ;;
esac
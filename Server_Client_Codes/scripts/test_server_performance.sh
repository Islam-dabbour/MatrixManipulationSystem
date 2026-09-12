#!/bin/bash
#
# Starts the server and creates a chosen number of clients using chosen matrix dimensions.

set -euo pipefail

if [[ $# -ne 4 ]]; then
    printf 'Usage: %s <number_of_clients> <rowsA> <columnsA> <columnsB>\n' "$0" >&2
    exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NUM_CLIENTS="$1"
ROWS_A="$2"
COLUMNS_A="$3"
COLUMNS_B="$4"
PORT=1450
SERVER_PID=""
CLIENT_PIDS=()

cleanup() {
    for client_pid in "${CLIENT_PIDS[@]}"; do
        if kill -0 "${client_pid}" 2>/dev/null; then
            kill "${client_pid}" 2>/dev/null || true
        fi
    done

    if [[ -n "${SERVER_PID}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill "${SERVER_PID}" 2>/dev/null || true
        wait "${SERVER_PID}" 2>/dev/null || true
    fi
}
trap cleanup INT TERM EXIT

cd "${PROJECT_ROOT}" || exit 1
./server "${PORT}" >/dev/null 2>&1 &
SERVER_PID=$!
sleep 1

printf '[TEST] Starting %s clients with matrices %sx%s and %sx%s on port %s\n' \
    "${NUM_CLIENTS}" "${ROWS_A}" "${COLUMNS_A}" "${COLUMNS_A}" "${COLUMNS_B}" "${PORT}"

for ((client = 1; client <= NUM_CLIENTS; client++)); do
    {
        #printf 'testing'
        printf '1\n%s\n%s\n%s\n2\n1\n6\n-1\n' \
            "${ROWS_A}" "${COLUMNS_A}" "${COLUMNS_B}"
    } | "${PROJECT_ROOT}/clinet" "${PORT}" >/dev/null 2>&1 &
    CLIENT_PIDS+=("$!")
done

wait "${CLIENT_PIDS[@]}"
printf '[TEST] All clients completed. Server details are in %s/server.log\n' "${PROJECT_ROOT}"

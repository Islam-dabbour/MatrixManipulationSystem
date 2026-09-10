#!/bin/bash
#
# search_logs.sh
# Performs advanced regex/keyword searches across log files and result
# directories. Supports two modes:
#
#   direct <pattern> <directory>          -> immediate recursive regex search
#   daemon-start / daemon-query / daemon-stop
#       -> mirrors the C server's request/response FIFO pattern: a background
#          "search worker" process listens on a named pipe for search
#          requests and writes matches to a response FIFO, demonstrating the
#          same IPC design used by multiplication_worker/transposition_worker
#          in shell-script form.

set -euo pipefail

SEARCH_REQUEST_FIFO="/tmp/search_request_fifo"
SEARCH_RESPONSE_FIFO="/tmp/search_response_fifo"
WORKER_PID_FILE="/tmp/search_worker.pid"

MODE="${1:-}"

# ---------------------------------------------------------------------------
# Direct mode: one-shot advanced regex search
# ---------------------------------------------------------------------------
direct_search() {
    local pattern="$1"
    local dir="$2"

    echo "[SEARCH] Pattern   : ${pattern}"
    echo "[SEARCH] Directory : ${dir}"
    echo "[SEARCH] ------------------------------------"

    grep -E -r -n -i \
        --include="*.log" --include="*.txt" --include="*.csv" \
        "${pattern}" "${dir}" || echo "[SEARCH] No matches found."
}

# ---------------------------------------------------------------------------
# Daemon mode: FIFO-based request/response search worker (IPC demonstration)
# ---------------------------------------------------------------------------
start_daemon() {
    rm -f "${SEARCH_REQUEST_FIFO}" "${SEARCH_RESPONSE_FIFO}"
    mkfifo "${SEARCH_REQUEST_FIFO}"
    mkfifo "${SEARCH_RESPONSE_FIFO}"

    (
        while true; do
            if read -r request_line < "${SEARCH_REQUEST_FIFO}"; then
                [ "${request_line}" = "STOP" ] && break

                local search_pattern search_dir
                search_pattern="$(cut -d'|' -f1 <<< "${request_line}")"
                search_dir="$(cut -d'|' -f2 <<< "${request_line}")"

                {
                    grep -E -r -n -i "${search_pattern}" "${search_dir}" 2>/dev/null \
                        || echo "NO_MATCHES"
                    echo "END_OF_RESULTS"
                } > "${SEARCH_RESPONSE_FIFO}"
            fi
        done
    ) &

    echo $! > "${WORKER_PID_FILE}"
    echo "[SEARCH] Search worker started (PID $(cat "${WORKER_PID_FILE}"))."
}

query_daemon() {
    local pattern="$1"
    local dir="$2"

    echo "${pattern}|${dir}" > "${SEARCH_REQUEST_FIFO}"
    cat "${SEARCH_RESPONSE_FIFO}"
}

stop_daemon() {
    echo "STOP" > "${SEARCH_REQUEST_FIFO}" || true
    [ -f "${WORKER_PID_FILE}" ] && kill "$(cat "${WORKER_PID_FILE}")" 2>/dev/null || true
    rm -f "${SEARCH_REQUEST_FIFO}" "${SEARCH_RESPONSE_FIFO}" "${WORKER_PID_FILE}"
    echo "[SEARCH] Search worker stopped."
}

case "${MODE}" in
    direct)        direct_search "$2" "$3" ;;
    daemon-start)  start_daemon ;;
    daemon-query)  query_daemon "$2" "$3" ;;
    daemon-stop)   stop_daemon ;;
    *)
        echo "Usage: $0 direct <pattern> <dir>"
        echo "       $0 daemon-start"
        echo "       $0 daemon-query <pattern> <dir>"
        echo "       $0 daemon-stop"
        exit 1
        ;;
esac
#!/bin/bash
#
# run_benchmark.sh
# Master automation script. Launches the server, drives several concurrent
# client sessions with pre-generated matrix-operation inputs, verifies the
# correctness of each result, records timing statistics, and organizes every
# artifact produced into the structured results directory created by
# setup_environment.sh.

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUN_ID="$(date +%Y%m%d_%H%M%S)"
RESULTS_ROOT="${PROJECT_ROOT}/results/${RUN_ID}"
PORT=3650
NUM_CLIENTS=1

mkdir -p "${RESULTS_ROOT}/logs" \
         "${RESULTS_ROOT}/computation_results" \
         "${RESULTS_ROOT}/timing_stats" \
         "${RESULTS_ROOT}/reports" \
         "${RESULTS_ROOT}/diagnostics"
printf '%s\n' "${RESULTS_ROOT}" > "${PROJECT_ROOT}/.last_run"

echo "[BENCH] Using results directory: ${RESULTS_ROOT}"

SERVER_PID=""
CLIENT_PIDS=()

cleanup() {
    for client_pid in "${CLIENT_PIDS[@]}"; do
        kill "${client_pid}" 2>/dev/null || true
    done

    if [[ -n "${SERVER_PID}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill -- "-${SERVER_PID}" 2>/dev/null || kill "${SERVER_PID}" 2>/dev/null || true
        wait "${SERVER_PID}" 2>/dev/null || true
    fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 148' TSTP
trap 'cleanup; exit 148' TSTP

# ---------------------------------------------------------------------------
# 1. Launch the server in the background
# ---------------------------------------------------------------------------
cd "${PROJECT_ROOT}"
if ss -ltn | awk -v port=":${PORT}" '$4 ~ port"$" { found=1 } END { exit !found }'; then
    echo "[BENCH] Port ${PORT} is already in use; refusing to attach clients to an existing server." >&2
    echo "[BENCH] Stop the existing benchmark server and retry." >&2
    exit 1
fi
setsid ./server "${PORT}" > "${RESULTS_ROOT}/logs/server_stdout.log" 2>&1 &
SERVER_PID=$!
echo "[BENCH] Server started (PID ${SERVER_PID}) on port ${PORT}"
sleep 1   # allow the socket to bind before clients connect

server_ready=false

for attempt in {1..30}; do
    if ! kill -0 "${SERVER_PID}" 2>/dev/null; then
        echo "[BENCH] Server failed to start. See ${RESULTS_ROOT}/logs/server_stdout.log" >&2
        exit 1
    fi

    if ss -ltn | awk -v port=":${PORT}" '$4 ~ port"$" { found=1 } END { exit !found }'; then
        server_ready=true
        break
    fi

    sleep 1
done

if [[ "${server_ready}" != true ]]; then
    echo "[BENCH] Server did not start listening on port ${PORT}." >&2
    echo "[BENCH] See ${RESULTS_ROOT}/logs/server_stdout.log" >&2
    exit 1
fi

echo "[BENCH] Server is listening on port ${PORT}"

# ---------------------------------------------------------------------------
# 2. Generate one input script per client and launch them concurrently
# ---------------------------------------------------------------------------
run_single_client() {
    local client_index="$1"
    local rows_a=2
    local cols_a=2
    local cols_b=2

    local input_file="${RESULTS_ROOT}/diagnostics/client_${client_index}_input.txt"
    local output_file="${RESULTS_ROOT}/computation_results/client_${client_index}_output.txt"
    local timing_file="${RESULTS_ROOT}/timing_stats/client_${client_index}_timing.txt"

    {
        echo "1"          # fill matrices
        echo "${rows_a}"
        echo "${cols_a}"
        echo "${cols_b}"
        echo "2"          # open services list
        echo "1"          # multiplication
        echo "-1"          # exit
    } > "${input_file}"

    local start_time end_time
    start_time=$(date +%s.%N)

    ./client "${PORT}" < "${input_file}" > "${output_file}" 2>&1

    end_time=$(date +%s.%N)
    echo "$(echo "${end_time} - ${start_time}" | bc)" > "${timing_file}"

    echo "[BENCH] Client ${client_index} finished (${rows_a}x${cols_a} * ${cols_a}x${cols_b})."
}

for i in $(seq 1 "${NUM_CLIENTS}"); do
    run_single_client "${i}" &
    CLIENT_PIDS+=("$!")
done

wait "${CLIENT_PIDS[@]}"

echo "[BENCH] All ${NUM_CLIENTS} concurrent client sessions completed."

# ---------------------------------------------------------------------------
# 3. Verify correctness of each client's result
# ---------------------------------------------------------------------------
"${PROJECT_ROOT}/scripts/verify_result.sh" "${RESULTS_ROOT}"

# ---------------------------------------------------------------------------
# 4. Aggregate timing statistics into a single report
# ---------------------------------------------------------------------------
{
    echo "Client, Time (seconds)"
    for i in $(seq 1 "${NUM_CLIENTS}"); do
        echo "${i}, $(cat "${RESULTS_ROOT}/timing_stats/client_${i}_timing.txt")"
    done
} > "${RESULTS_ROOT}/reports/timing_summary.csv"

echo "[BENCH] Timing summary written to reports/timing_summary.csv"

# ---------------------------------------------------------------------------
# 5. Copy the server log into the run's results directory
# ---------------------------------------------------------------------------
cp "${PROJECT_ROOT}/server.log" "${RESULTS_ROOT}/logs/server.log" 2>/dev/null || true

"${PROJECT_ROOT}/scripts/permissions.sh" lock "${RESULTS_ROOT}/logs"

echo "[BENCH] Benchmark run complete. Artifacts under: ${RESULTS_ROOT}"
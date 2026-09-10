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
RESULTS_ROOT="$(cat "${PROJECT_ROOT}/.last_run")"
PORT=5050
NUM_CLIENTS=5

echo "[BENCH] Using results directory: ${RESULTS_ROOT}"

# ---------------------------------------------------------------------------
# 1. Launch the server in the background
# ---------------------------------------------------------------------------
cd "${PROJECT_ROOT}"
./server "${PORT}" > "${RESULTS_ROOT}/logs/server_stdout.log" 2>&1 &
SERVER_PID=$!
echo "[BENCH] Server started (PID ${SERVER_PID}) on port ${PORT}"
sleep 1   # allow the socket to bind before clients connect

# ---------------------------------------------------------------------------
# 2. Generate one input script per client and launch them concurrently
# ---------------------------------------------------------------------------
run_single_client() {
    local client_index="$1"
    local rows_a=$((RANDOM % 20 + 2))
    local cols_a=$((RANDOM % 20 + 2))
    local cols_b=$((RANDOM % 20 + 2))

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
        echo "6"          # back
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
done
wait

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
# 5. Shut down the server and copy its log into the run's results directory
# ---------------------------------------------------------------------------
kill "${SERVER_PID}" 2>/dev/null || true
wait "${SERVER_PID}" 2>/dev/null || true
cp "${PROJECT_ROOT}/server.log" "${RESULTS_ROOT}/logs/server.log" 2>/dev/null || true

"${PROJECT_ROOT}/scripts/permissions.sh" lock "${RESULTS_ROOT}/logs"

echo "[BENCH] Benchmark run complete. Artifacts under: ${RESULTS_ROOT}"
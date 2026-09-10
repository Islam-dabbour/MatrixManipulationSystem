#!/bin/bash
#
# setup_environment.sh
# Prepares the execution environment for the distributed matrix-processing
# system: creates the output directory hierarchy, compiles all binaries,
# and applies the correct file permissions to executables and FIFOs.

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUN_ID="$(date +%Y%m%d_%H%M%S)"
RESULTS_ROOT="${PROJECT_ROOT}/results/${RUN_ID}"

echo "[SETUP] Project root      : ${PROJECT_ROOT}"
echo "[SETUP] Run identifier    : ${RUN_ID}"

# ---------------------------------------------------------------------------
# 1. Structured output directory hierarchy
# ---------------------------------------------------------------------------
mkdir -p "${RESULTS_ROOT}/logs"
mkdir -p "${RESULTS_ROOT}/computation_results"
mkdir -p "${RESULTS_ROOT}/timing_stats"
mkdir -p "${RESULTS_ROOT}/reports"
mkdir -p "${RESULTS_ROOT}/diagnostics"

echo "${RESULTS_ROOT}" > "${PROJECT_ROOT}/.last_run"
echo "[SETUP] Output hierarchy created under: ${RESULTS_ROOT}"

# ---------------------------------------------------------------------------
# 2. Compilation
# ---------------------------------------------------------------------------
echo "[SETUP] Compiling binaries..."
cd "${PROJECT_ROOT}"

gcc -o server            server.c logger_utils.c  -lpthread
gcc -o client             clinet.c
gcc -o multiplication_worker  multiplication_worker.c  -lpthread
gcc -o transposition_worker   transposition_worker.c   -lpthread
gcc -o average_worker         average_worker.c         -lpthread
gcc -o logger_process         logger.c
gcc -o time_process            time_process.c

echo "[SETUP] Compilation completed successfully."

# ---------------------------------------------------------------------------
# 3. Permissions
# ---------------------------------------------------------------------------
"${PROJECT_ROOT}/scripts/permissions.sh" apply "${PROJECT_ROOT}"

echo "[SETUP] Environment ready. Results directory: ${RESULTS_ROOT}"
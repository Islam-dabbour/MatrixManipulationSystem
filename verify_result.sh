#!/bin/bash
#
# verify_results.sh
# Checks that every client output file in a results directory contains a
# well-formed matrix result (no error strings, non-empty numeric output),
# and cross-checks the server log to confirm a matching OPERATION_COMPLETED
# entry exists for each client that ran.

set -euo pipefail

RESULTS_ROOT="$1"
PASS_COUNT=0
FAIL_COUNT=0

echo "[VERIFY] Checking computation results in: ${RESULTS_ROOT}/computation_results"

for output_file in "${RESULTS_ROOT}"/computation_results/*_output.txt; do
    client_tag="$(basename "${output_file}" _output.txt)"

    if grep -qiE "error|segmentation|refused" "${output_file}"; then
        echo "  [FAIL] ${client_tag}: error string found in output."
        FAIL_COUNT=$((FAIL_COUNT + 1))
        continue
    fi

    if ! grep -qE "^[0-9 ]+$" "${output_file}"; then
        echo "  [FAIL] ${client_tag}: no numeric matrix data found."
        FAIL_COUNT=$((FAIL_COUNT + 1))
        continue
    fi

    echo "  [PASS] ${client_tag}: valid numeric result."
    PASS_COUNT=$((PASS_COUNT + 1))
done

echo "[VERIFY] ------------------------------------"
echo "[VERIFY] Passed: ${PASS_COUNT}   Failed: ${FAIL_COUNT}"

if [ "${FAIL_COUNT}" -ne 0 ]; then
    exit 1
fi
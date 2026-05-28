#!/usr/bin/env bash
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "Error: The script must be sourced. please run: 'source $0'"
    exit 1
fi

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PATH="${SCRIPT_DIR}:${PATH}"

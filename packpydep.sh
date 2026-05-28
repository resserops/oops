#! /usr/bin/env bash
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
mkdir -p ${SCRIPT_DIR}/pydep
pip download -d ${SCRIPT_DIR}/pydep -r ${SCRIPT_DIR}/requirements.txt

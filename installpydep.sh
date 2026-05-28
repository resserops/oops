#! /usr/bin/env bash
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
pip install --no-index --find-links=${SCRIPT_DIR}/offline_packages -r requirements.txt

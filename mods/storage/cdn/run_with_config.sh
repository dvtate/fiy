#!/bin/bash

# Set these as needed

# Database file
export DB_PATH=/tmp/cdn/db.db3

# Where to store files
export DATA_DIR=/tmp/cdn/assets

# Bearer token to expect from API users
export BEARER_TOKEN=abc

# Max capacity for DATA_DIR
export MAX_STORAGE=10G

# Listen address
export ADDRESS=0.0.0.0

# Listen port
export PORT=6333

# Number of threads to use
export THREADS="$(nproc)"

if [[ "$1" == "debug" ]]; then
    shift
    gdb --ex run --args ./fiy_cdn "$@"
else
    ./fiy_cdn "$@"
fi
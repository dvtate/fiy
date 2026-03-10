#!/bin/bash

# Set these as needed
export DB_PATH=/tmp/cdn/db.db3
export DATA_DIR=/tmp/cdn/assets
export BEARER_TOKEN=abc
export MAX_STORAGE=10G
export ADDRESS=0.0.0.0
export PORT=6333
export THREADS="$(nproc)"

./fiy_cdn
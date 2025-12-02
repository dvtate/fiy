#!/bin/bash
set -e

command -v npm >/dev/null 2>&1 || { echo >&2 "npm is required but not installed.  Aborting."; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo >&2 "cmake is required but not installed.  Aborting."; exit 1; }

function build_cpp {
    local build_type=$1
    mkdir build | :
    cd build
    if [ "$build_type" = "debug" ]; then
        cmake -DCMAKE_BUILD_TYPE=Debug ..
    else
        cmake ..
    fi
    make -j`nproc`
    cd ..
}

# Build contacts mod which has uses TS/JS+webpack
# TODO make a separate cmake module for it and run npm commands from it
function build_contacts_mod {
    cd mods/contacts/frontend
    npm install
    npm run build
    cd ../../..
}

# Run build tasks in parallel
build_cpp "$1" &
build_contacts_mod &
wait

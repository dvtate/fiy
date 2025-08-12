#!/bin/bash
set -e

function build_cpp {
  mkdir build | :
  cd build
  cmake ..
  make -j`nproc`
  cd ..
}

# Build contacts mod which has uses TS/JS+webpack
function build_contacts_mod {
  cd mods/contacts/frontend
  npm install
  npm run build
  cd ../../..
}

# Run build tasks in parallel
build_cpp &
build_contacts_mod &
wait
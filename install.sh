#!/bin/bash
set -e

#
# This script is a temporary way to install the fediy protocol server.
# Eventually this will be replaced by an installation package.
#

###
# Safety checks
###

command -v sqlite3 >/dev/null 2>&1 || { echo >&2 "sqlite3 is required but not installed.  Aborting."; exit 1; }
command -v openssl >/dev/null 2>&1 || { echo >&2 "openssl is required but not installed.  Aborting."; exit 1; }

if [[ $EUID -ne 0 ]]; then
   echo "Not root, make sure you have access to install location"
fi

###
# Prompt user
###

prompt_Y_n () {
    ESC_BOLD='\033[1m'
    ESC_NC='\033[0m'
    printf "${ESC_BOLD}$1${ESC_NC} (Y/n): "
    read
    if [[ -z "$REPLY" ]] || [[ "${REPLY:0:1}" == "y" ]] || [[ "${REPLY:0:1}" == "Y" ]]; then
        return 0
    fi
    return 1
}
prompt_y_N () {
    ESC_BOLD='\033[1m'
    ESC_NC='\033[0m'
    printf "${ESC_BOLD}$1${ESC_NC} (y/N): "
    read
    if [[ -z "$REPLY" ]]; then
        return 1
    fi
    if [[ "${REPLY:0:1}" == "y" ]] || [[ "${REPLY:0:1}" == "Y" ]]; then
        return 0
    fi
    return 1
}

if [ ! -f build/protocol_server ]; then
    echo "Project not built yet";
    read -p "Press Enter to build Fediy or ctrl+c to cancel..."
    ./build.sh
    echo "Build finished."
fi

declare DEVEL_INSTALL
if prompt_y_N "Is this a development setup?"; then
    DEVEL_INSTALL=1
else
    DEVEL_INSTALL=0
fi

declare INSTALL_PATH
read -r -p "Enter install location (default: /opt/fediy) " INSTALL_PATH
if [[ -z "$INSTALL_PATH" ]]; then
    INSTALL_PATH="/opt/fediy"
fi

declare PORT
read -r -p "Enter a free port number (default: 8848) " PORT
if [[ -z "$PORT" ]]; then
    PORT=8848
fi

declare HOSTNAME
read -r -p "Enter the hostname for this instance (default: localhost:8848) " HOSTNAME
if [[ -z "$HOSTNAME" ]]; then
    HOSTNAME=localhost:8848
fi

INSTALL_SYSTEMD_UNIT=0
if prompt_y_N "Install sample systemd unit file?"; then
    INSTALL_SYSTEMD_UNIT=1
fi

echo

###
# Install
###

# Make data dir
mkdir "$INSTALL_PATH" || echo "Couldn't mkdir $INSTALL_PATH"
mkdir "$INSTALL_PATH/mods"
mkdir "$INSTALL_PATH/auth"
chmod -R 777 "$INSTALL_PATH"

SALT="$(tr -dc A-Za-z0-9 </dev/urandom | head -c 36)"

declare CONCURRENCY
if [ "$DEVEL_INSTALL" -eq 1 ]; then
    CONCURRENCY="1"
else
    CONCURRENCY="$(nproc)"
fi

# Make config.ini
echo "
hostname=$HOSTNAME
port=$PORT
data_dir=$INSTALL_PATH
salt=$SALT
concurrency=$CONCURRENCY

public_key=$INSTALL_PATH/auth/pubkey.crt
private_key=$INSTALL_PATH/auth/privkey.pem
" > "$INSTALL_PATH/config.ini"

# Initialize database
sqlite3 "$INSTALL_PATH/db.db3" < ./protocol/db.sql
echo "Initialized database."

declare CP_INSTALL_FLAG
if [ "$DEVEL_INSTALL" -eq 1 ]; then
    CP_INSTALL_FLAG="-s"
else
    CP_INSTALL_FLAG=""
fi

###
# Install builtin mods
###
if [ "$DEVEL_INSTALL" -eq 1 ]; then
    mkdir "$INSTALL_PATH/mods/demo_cpp"
    echo '{"name": "Demo",
  "description": "This is an example mod made with C++",
  "icon": null,
  "enabled": true,
  "connector": "shared_object",
  "path": "test"
}' > "$INSTALL_PATH/mods/demo_cpp/module.json"
    cp $CP_INSTALL_FLAG "$(realpath ./build/libdemo_mod_cpp.so)" "$INSTALL_PATH/mods/demo_cpp/module.so"
    echo "Installed C++ testing mod."
fi

mkdir "$INSTALL_PATH/mods/mail"
cp $CP_INSTALL_FLAG "$(realpath ./mods/mail/module.json)" "$INSTALL_PATH/mods/mail/module.json"
cp $CP_INSTALL_FLAG "$(realpath ./build/libdemo_mod_mail.so)" "$INSTALL_PATH/mods/mail/module.so"
echo "Installed Mail mod."

mkdir "$INSTALL_PATH/mods/contacts"
mkdir "$INSTALL_PATH/mods/contacts/assets"
cp $CP_INSTALL_FLAG "$(realpath ./mods/contacts/module.json)" "$INSTALL_PATH/mods/contacts/"
cp $CP_INSTALL_FLAG "$(realpath ./build/libcontacts_mod.so)" "$INSTALL_PATH/mods/contacts/module.so"
cp $CP_INSTALL_FLAG "$(realpath ./mods/contacts/frontend/dist)"/* "$INSTALL_PATH/mods/contacts/assets"
sqlite3 "$INSTALL_PATH/mods/contacts/db.db3" < ./mods/contacts/db.sql
echo "Installed Contacts mod."

mkdir "$INSTALL_PATH/mods/git"
mkdir "$INSTALL_PATH/mods/git/repos"
mkdir "$INSTALL_PATH/mods/git/static"
cp $CP_INSTALL_FLAG "$(realpath ./mods/git/module.json)" "$INSTALL_PATH/mods/git/."
cp $CP_INSTALL_FLAG "$(realpath ./build/libgit_mod.so)" "$INSTALL_PATH/mods/git/module.so"
cp $CP_INSTALL_FLAG "$(realpath ./mods/git/frontend)"/* "$INSTALL_PATH/mods/git/static"
sqlite3 "$INSTALL_PATH/mods/git/db.db3" < ./mods/git/db.sql
echo "Installed Git mod."


# Install protocol server
cp $CP_INSTALL_FLAG "$(realpath ./build/protocol_server)" "$INSTALL_PATH/server"
cp $CP_INSTALL_FLAG "$(realpath ./build/libfediymod.so)" "$INSTALL_PATH/libfediymod.so"
echo "Installed binaries."

if [ "$DEVEL_INSTALL" -eq 1 ]; then
    cp -r $CP_INSTALL_FLAG "$(realpath ./portal_frontend)" "$INSTALL_PATH/pages"
else
    mkdir "$INSTALL_PATH/pages"
    cp ./portal_frontend/* "$INSTALL_PATH/pages"
fi
echo "Installed Portal Frontend."

# systemd unit file
echo "
[Unit]
Description=FedIY Protocol Server

[Service]
Type=simple
ExecStart=$INSTALL_PATH/server $INSTALL_PATH/config.ini
#User=$(whoami) # replace this and uncomment
# For security, it's recommended to run the server as a dedicated non-privileged user
" > "$INSTALL_PATH/fediy.service"
if [ "$INSTALL_SYSTEMD_UNIT" -eq 1 ]; then
    mv "$INSTALL_PATH/fediy.service" "$INSTALL_PATH/fediy.service.sample"
    cp "$INSTALL_PATH/fediy.service.sample" /etc/systemd/system/fediy.service
    echo
    echo "To enable and start the fediy systemd daemon run"
    echo "sudo systemctl daemon-reload"
    echo "sudo systemctl edit fediy.service"
    echo "sudo systemctl enable --now fediy.service"
    echo
else
    echo "Wrote sample systemd unit file to: $INSTALL_PATH/fediy.service"
fi

## Generate and install server keys
echo "Generating server key pair..."
openssl genrsa -out "$INSTALL_PATH/auth/_privkey.pem" 2048
openssl rsa -in "$INSTALL_PATH/auth/_privkey.pem" -pubout -out "$INSTALL_PATH/auth/pubkey.crt"
# Convert to pkcs8 format ... probably not necessary
openssl pkcs8 -topk8 -inform PEM -outform PEM -nocrypt -in "$INSTALL_PATH/auth/_privkey.pem" -out "$INSTALL_PATH/auth/privkey.pem"
echo "Generated server key pair."

# FIXME
chmod -R 777 "$INSTALL_PATH"

###
# Success
###

echo
echo
echo "Fediy installed to $INSTALL_PATH"
echo
echo "To start the fediy protocol server, run"
echo "$INSTALL_PATH/server $INSTALL_PATH/config.ini"
echo

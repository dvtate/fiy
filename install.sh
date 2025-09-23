#!/bin/bash
set -e

command -v sqlite3 >/dev/null 2>&1 || { echo >&2 "sqlite3 is required but not installed.  Aborting."; exit 1; }
command -v gpg >/dev/null 2>&1 || { echo >&2 "gpg is required but not installed.  Aborting."; exit 1; }


if [[ $EUID -ne 0 ]]; then
   echo "Not root, make sure you have access to install location"
fi

if [ ! -f build/protocol_server ]; then
    echo "Project not built yet";
    read -p "Press Enter to build Fediy or ctrl+c to cancel..."
    ./build.sh
    echo "Build finished."
fi

declare DEVEL_INSTALL

read -p "Is this a development setup? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
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

echo


# Make data dir
mkdir "$INSTALL_PATH" || echo "Couldn't mkdir $INSTALL_PATH"
mkdir "$INSTALL_PATH/mods"
mkdir "$INSTALL_PATH/auth"
chmod -R 777 "$INSTALL_PATH"

SALT="$(tr -dc A-Za-z0-9 </dev/urandom | head -c 36)"

declare CONCURRENCY
if [[ ]]
# Make config.ini
echo "
hostname=$HOSTNAME
port=$PORT
data_dir=$INSTALL_PATH
salt=$SALT
" > "$INSTALL_PATH/config.ini"

# Initialize database
sqlite3 "$INSTALL_PATH/db.db3" < ./protocol/db.sql
echo "Initialized database"

declare CP_INSTALL_FLAG
if [ "$DEVEL_INSTALL" -eq 1 ]; then
    CP_INSTALL_FLAG="-s"
else
    CP_INSTALL_FLAG=""
fi

# Install builtin mods
mkdir "$INSTALL_PATH/mods/mail"
cp $CP_INSTALL_FLAG "$(realpath ./mods/mail/module.json)" "$INSTALL_PATH/mods/mail/module.json"
cp $CP_INSTALL_FLAG "$(realpath ./build/libdemo_mod_mail.so)" "$INSTALL_PATH/mods/mail/module.so"
echo "Installed Mail mod"

mkdir "$INSTALL_PATH/mods/contacts"
mkdir "$INSTALL_PATH/mods/contacts/assets"
cp $CP_INSTALL_FLAG "$(realpath ./mods/contacts/module.json)" "$INSTALL_PATH/mods/contacts/"
cp $CP_INSTALL_FLAG "$(realpath ./build/libcontacts_mod.so)" "$INSTALL_PATH/mods/contacts/module.so"
cp $CP_INSTALL_FLAG "$(realpath ./mods/contacts/frontend/dist)"/* "$INSTALL_PATH/mods/contacts/assets"
sqlite3 "$INSTALL_PATH/mods/contacts/db.db3" < ./mods/contacts/db.sql
echo "Installed Contacts mod"

# TODO install other mods

# Install protocol server
cp $CP_INSTALL_FLAG "$(realpath ./build/protocol_server)" "$INSTALL_PATH/server"
cp $CP_INSTALL_FLAG "$(realpath ./build/libfediymod.so)" "$INSTALL_PATH/libfediymod.so"
echo "Installed binaries"

if [ "$DEVEL_INSTALL" -eq 1 ]; then
    cp -r $CP_INSTALL_FLAG "$(realpath ./portal_frontend)" "$INSTALL_PATH/pages"
else
    mkdir "$INSTALL_PATH/pages"
    cp ./portal_frontend/* "$INSTALL_PATH/pages"
fi
echo "Installed Portal Frontend"

# Generate and install GPG keys
echo "Generating server key pair..."
GPG_BATCH_CONF="$(mktemp)"
GPG_TMP_HOME="$(mktemp -d)"
echo "
%echo Generating a GPG key
Key-Type: RSA
Key-Length: 2048
Name-Real: Fediy Instance $HOSTNAME
Name-Email: admin@example.com
Expire-Date: 0
Passphrase: not-used
%no-protection
%commit
%echo Done
" > "$GPG_BATCH_CONF"
gpg --batch --homedir "$GPG_TMP_HOME" --generate-key "$GPG_BATCH_CONF"
gpg --homedir "$GPG_TMP_HOME" --armor --export-secret-keys > "$INSTALL_PATH/auth/_privkey.gpg"
gpg --homedir "$GPG_TMP_HOME" --armor --export > "$INSTALL_PATH/auth/key"
rm "$GPG_BATCH_CONF"
rm -rf "$GPG_TMP_HOME"
echo "Generated server key pair."

chmod -R 777 "$INSTALL_PATH"

echo
echo
echo "Fediy installed to $INSTALL_PATH"
echo
echo "To start the fediy protocol server, run"
echo "$INSTALL_PATH/server $INSTALL_PATH/config.ini"
echo
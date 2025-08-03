#!/bin/bash

# this file is only temporary way to install fediy for testing

sudo rm -rf /opt/fediy
sudo mkdir /opt/fediy
sudo mkdir /opt/fediy/apps
sudo mkdir /opt/fediy/pages
sudo chmod -R 777 /opt/fediy/

touch /opt/fediy/config.ini
# Prompt user for relevant info and populate the file

cp -R portal_frontend/* /opt/fediy/pages/

cat protocol/db.sql | sqlite3 /opt/fediy/db.db3

# Install builtin apps
mkdir /opt/fediy/apps/
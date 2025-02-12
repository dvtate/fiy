#!/bin/bash

# this file is only temporary way to install fediy for testing

sudo rm -rf /opt/fediy
sudo mkdir /opt/fediy
sudo mkdir /opt/fediy/apps
sudo mkdir /opt/fediy/page_templates
sudo chmod -R 777 /opt/fediy/

touch /opt/fediy/config.ini
# Prompt user for relevant info and populate the file

cp -R portal_frontend/* /opt/fediy/page_templates/

cat protocol/db.sql | sqlite3 /opt/fediy/db.db3


#!/bin/bash

mkdir dist || :
rm dist/*

cp node_modules/font-awesome/fonts/* dist
cp node_modules/font-awesome/css/font-awesome.min.css dist/font-awesome.css


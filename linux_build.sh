#!/bin/bash

# run using something similar to this:
# bash linux_build.sh python3.5m
# puts result in linux/x64

# requires:
# sudo apt-get install python3-dev

# build executable
mkdir -p linux/x64
export PYTHON_LIB=$1
qmake NionUILauncher.pro
make clean
make
# copy executable to target directory
mv NionUILauncher linux/x64
cp bootstrap.py linux/x64/bootstrap.py

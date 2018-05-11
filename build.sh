#!/bin/bash

# debugging lines
# echo "RUNNING"
# echo $PREFIX
# pwd
# ls -R

mkdir -p "$PREFIX/bin"

if [ -e NionUILauncher-Mac.zip ]
then
    unzip NionUILauncher-Mac.zip -d "$PREFIX/bin"
fi

if [ -e NionUILauncher-Linux.zip ]
then
    unzip NionUILauncher-Linux.zip -d "$PREFIX/bin/NionUILauncher"
fi

python -m pip install --no-deps --ignore-installed .

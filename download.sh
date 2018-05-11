#!/bin/sh

# note: pass the version; but also change it in each of the setup scripts.

version=$1

curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Mac.zip
curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Linux.zip
curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Windows.zip

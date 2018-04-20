#!/bin/sh

# note: pass the version; but also change it in each of the setup scripts.

version=$1

curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Mac.zip
curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Linux.zip
curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Windows.zip

rm -rf dist

rm -rf nui/macosx build nui.egg-info
mkdir -p nui/macosx
unzip NionUILauncher-Mac.zip -d nui/macosx
python setup_macos.py bdist_wheel --plat-name macosx-10.11-x86_64
rm -rf nui/macosx

rm -rf nui/linux build nui.egg-info
mkdir -p nui/linux
unzip NionUILauncher-Linux.zip -d nui/linux
python setup_linux.py bdist_wheel --plat-name linux-x86_64
rm -rf nui/linux

rm -rf nui/windows build nui.egg-info
mkdir -p nui/windows
unzip NionUILauncher-Windows.zip -d nui/windows
python setup_windows.py bdist_wheel --plat-name win-amd64
rm -rf nui/windows

rm -rf NionUILauncher-Mac.zip
rm -rf NionUILauncher-Linux.zip
rm -rf NionUILauncher-Windows.zip

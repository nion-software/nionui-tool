#!/bin/sh

# note: pass the version; but also change it in each of the setup scripts.

version=$1

curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Mac.zip
curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Linux.zip
curl -L -O https://github.com/nion-software/nionui-launcher/releases/download/v$version/NionUILauncher-Windows.zip

rm -rf dist

rm -rf nion/nionui_tool/macosx build nionui_tool.egg-info
mkdir -p nion/nionui_tool/macosx
unzip NionUILauncher-Mac.zip -d nion/nionui_tool/macosx
python setup_macos.py bdist_wheel --plat-name macosx-10.11-x86_64
rm -rf nion/nionui_tool/macosx

rm -rf nion/nionui_tool/linux build nionui_tool.egg-info
mkdir -p nion/nionui_tool/linux
unzip NionUILauncher-Linux.zip -d nion/nionui_tool/linux
python setup_linux.py bdist_wheel --plat-name linux-x86_64
rm -rf nion/nionui_tool/linux

rm -rf nion/nionui_tool/windows build nionui_tool.egg-info
mkdir -p nion/nionui_tool/windows
unzip NionUILauncher-Windows.zip -d nion/nionui_tool/windows
python setup_windows.py bdist_wheel --plat-name win-amd64
rm -rf nion/nionui_tool/windows

rm -rf NionUILauncher-Mac.zip
rm -rf NionUILauncher-Linux.zip
rm -rf NionUILauncher-Windows.zip

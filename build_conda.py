"""
Build the conda nionui-tool packages.

Activate the conda build environment (Mac: pypi, Linux: build, Windows: build).

Run with python build_platform.py on each desired platform.

Upload resulting anaconda files to --user nion.
"""
import os
import pathlib
import requests
import shutil
import subprocess
import sys


def download_url(url, file_path):
    response = requests.get(url, stream=True)
    with open(file_path, "wb") as handle:
        handle.write(response.content)

build_dir = "conda_build"
project = "nionui-launcher"
project_root = "NionUILauncher"
tool = "nionui-tool"
version = "v0.3.19"

if sys.platform == "darwin":
    dir_path = f"{project_root}-Mac"
elif sys.platform == "win32":
    dir_path = f"{project_root}-Windows"
elif sys.platform == "linux":
    dir_path = f"{project_root}-Linux"
else:
    dir_path = None
    exit(0)

# everything will take place within pypi_build subdirectory
cwd = os.getcwd()
build_dir = pathlib.Path(cwd) / build_dir
if os.path.exists(build_dir):
    shutil.rmtree(build_dir)
os.makedirs(build_dir)
os.chdir(build_dir)

# download the tool projects
download_url(f"https://github.com/nion-software/{tool}/archive/master.zip", f"{tool}.zip")

# unzip the tool projects
if sys.platform == "win32":
    subprocess.call(["7z", "x", f"{tool}.zip", f"-o{tool}"])
else:
    subprocess.call(["unzip", "-q", f"{tool}.zip", "-d", f"{tool}"])

# download the release file
url = f"https://github.com/nion-software/{project}/releases/download/{version}/{dir_path}.zip"
file_path = f"{tool}/{tool}-master/{dir_path}.zip"
print(f"downloading {url} to {file_path}")
download_url(url, file_path)

# unzip the release file
if sys.platform == "win32":
    unzip_path = f"{tool}/{tool}-master/{dir_path}"
    print(f"unzipping {file_path} to {unzip_path}")
    subprocess.call(["7z", "x", file_path, f"-o{unzip_path}"])

# conda build
print(f"conda build {os.getcwd()}/{tool}/{tool}-master")
subprocess.call(["conda", "build", f"{tool}/{tool}-master"])

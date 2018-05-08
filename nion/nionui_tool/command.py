import os
import pkg_resources
import subprocess
import sys

def main():
    if sys.platform == "darwin":
        app_path = pkg_resources.resource_filename("nion.nionui_qt", "macosx/Nion UI Launcher.app")
        exe_path = os.path.join(app_path, "Contents", "MacOS", "Nion UI Launcher")
    elif sys.platform == "linux":
        app_dir = pkg_resources.resource_filename("nion.nionui_qt", "linux")
        exe_path = os.path.join(app_dir, "NionUILauncher")
    elif sys.platform == "win32":
        app_dir = pkg_resources.resource_filename("nion.nionui_qt", "windows")
        exe_path = os.path.join(app_dir, "NionUILauncher.exe")
    else:
        exe_path = None
    if exe_path:
        python_prefix = os.sys.prefix
        proc = subprocess.Popen([exe_path, python_prefix] + sys.argv[1:], universal_newlines=True)
        proc.communicate()

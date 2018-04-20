import os
import pkg_resources
import subprocess
import sys

def main():
    if sys.platform == "darwin":
        app_path = pkg_resources.resource_filename("nui", "macosx/Nion UI Launcher.app")
        exe_path = os.path.join(app_path, "Contents", "MacOS", "Nion UI Launcher")
    elif sys.platform == "linux":
        exe_path = pkg_resources.resource_filename("nui", "linux/NionUILauncher")
    elif sys.platform == "win32":
        exe_path = pkg_resources.resource_filename("nui", "windows/NionUILauncher.exe")
    else:
        exe_path = None
    if exe_path:
        python_prefix = os.sys.prefix
        proc = subprocess.Popen([exe_path, python_prefix] + sys.argv[1:], universal_newlines=True)
        proc.communicate()

import os
import subprocess
import sys
import sysconfig

def launch(argv):
    if sys.platform == "darwin":
        exe_path = os.path.join(sysconfig.get_paths()['scripts'], "Nion UI Launcher.app", "Contents", "MacOS", "Nion UI Launcher")
    elif sys.platform == "linux":
        exe_path = os.path.join(sysconfig.get_paths()['scripts'], "NionUILauncher", "NionUILauncher")
    elif sys.platform == "win32":
        exe_path = os.path.join(sys.exec_prefix, "Scripts", "NionUILauncher", "NionUILauncher.exe")
    else:
        exe_path = None
    if exe_path:
        python_prefix = sys.prefix
        proc = subprocess.Popen([exe_path, python_prefix] + argv[1:], universal_newlines=True)
        proc.communicate()

def main():
    launch(sys.argv)

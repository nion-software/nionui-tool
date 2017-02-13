import os
import re
import subprocess
import sys

path = sys.argv[1] # NionUILauncher.app/Contents/Frameworks
for directory_name, sub_directories, file_list in os.walk(path):
    for file_name in file_list:
        file_path = os.path.join(directory_name, file_name)
        try:
            result = subprocess.check_output(['otool', '-L', file_path]).decode('utf-8')
            results = result.split('\n')
            lines = []
            for line in results:
                if re.match('\t\/usr\/local\/Cellar\/.+\/lib\/', line) is not None:
                    print('Found at ' + line)
                    framework_path = re.search('\t\/usr\/local\/Cellar\/.+\/lib\/(\S+)', line).group(1)
                    original_path = re.search('\t(\S+)' + framework_path, line).group(1)
                    subprocess_args = ['install_name_tool', '-change', original_path + framework_path, '@rpath/' + framework_path, file_path]
                    subprocess.check_output(subprocess_args)
                    print(" ".join(subprocess_args))
        except subprocess.CalledProcessError:
            continue

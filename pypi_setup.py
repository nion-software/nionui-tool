"""
Run with python pypi_setup.py bdist_wheel.

The versions here, in nionui-tool, and in nionswift-tool must match.
"""
import os
import pathlib
import re
import requests
import setuptools
import shutil
import subprocess


def package_files(directory, prefix):
    prefixes = dict()
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            full_path = os.path.join(path, filename)
            if not os.path.islink(full_path):
                prefixes.setdefault(pathlib.Path(prefix) / pathlib.Path(*pathlib.Path(path).parts[1:]), list()).append(os.path.join(path, filename))
    return list(prefixes.items())


def download_url(url, file_path):
    response = requests.get(url, stream=True)
    with open(file_path, "wb") as handle:
        handle.write(response.content)


class BinaryDistribution(setuptools.Distribution):
    # force abi+platform in whl
    def has_data_files(self):
        return True
    def has_ext_modules(self):
        return True


from distutils.util import get_platform
from wheel.bdist_wheel import bdist_wheel as bdist_wheel_
from wheel.pep425tags import get_abbr_impl, get_impl_ver, get_abi_tag, get_platform, get_supported

platform = None
python_version = None
abi = None

class bdist_wheel(bdist_wheel_):
    def run(self):
        bdist_wheel_.run(self)

    def finalize_options(self):
        bdist_wheel_.finalize_options(self)
        self.universal = True
        self.plat_name_supplied = True
        global platform, python_version, abi
        self.plat_name = platform
        self.py_limited_api = python_version
        self.abi_tag = abi

    def get_tag(self):
        # bdist sets self.plat_name if unset, we should only use it for purepy
        # wheels if the user supplied it.
        if self.plat_name_supplied:
            plat_name = self.plat_name
        elif self.root_is_pure:
            plat_name = 'any'
        else:
            plat_name = self.plat_name or get_platform()
            if plat_name in ('linux-x86_64', 'linux_x86_64') and sys.maxsize == 2147483647:
                plat_name = 'linux_i686'
        plat_name = plat_name.replace('-', '_').replace('.', '_')

        if self.root_is_pure:
            if self.universal:
                impl = 'py2.py3'
            else:
                impl = self.python_tag
            tag = (impl, 'none', plat_name)
        else:
            impl_name = get_abbr_impl()
            impl_ver = get_impl_ver()
            impl = impl_name + impl_ver
            # We don't work on CPython 3.1, 3.0.
            if self.py_limited_api and (impl_name + impl_ver).startswith('cp3'):
                impl = self.py_limited_api
                abi_tag = 'abi3'
            else:
                abi_tag = str(get_abi_tag()).lower()
            abi_tag = self.abi_tag
            tag = (impl, abi_tag, plat_name)
            supported_tags = get_supported(
                supplied_platform=plat_name if self.plat_name_supplied else None)
            # XXX switch to this alternate implementation for non-pure:
            if not self.py_limited_api:
                assert tag == supported_tags[0], "%s != %s" % (tag, supported_tags[0])
            # assert tag in supported_tags, "would build wheel with unsupported tag {}".format(tag)
        return tag


def setup(tool_id, platform_, python_version_, abi_, dir_path, dest, project, version):

    file_path = f"{dir_path}.zip"

    # this will work, but since zip won't work, can't use it
    url = f"https://github.com/nion-software/{project}/releases/download/{version}/{dir_path}.zip"
    print(f"downloading {url} to {file_path}")
    download_url(url, file_path)

    # this won't work because it doesn't handle symlinks
    # import zipfile
    # with zipfile.ZipFile(file_path, 'r') as zip_ref:
    #     zip_ref.extractall(dir_path)

    if os.path.exists(dir_path):
        shutil.rmtree(dir_path)

    print(f"unzipping {file_path} to {dir_path}")
    subprocess.call(['unzip', '-q', file_path, '-d', dir_path])

    global platform, python_version, abi
    platform = platform_
    python_version = python_version_
    abi = abi_

    data_files = package_files(dir_path, dest)
    # print(data_files)

    version = re.sub('[^\d.]', '', version)

    print(f"setup {tool_id}-tool {version} nion.{tool_id}_tool")

    setuptools.setup(
        name= f"{tool_id}-tool",
        version=version,
        packages=[f"nion.{tool_id}_tool"],
        url='http://www.nion.com',
        license='Apache 2.0',
        author='Nion Software Team',
        author_email='software@nion.com',
        description='Python command line access to Nion UI Launcher',
        data_files=data_files,
        include_package_data=True,
        entry_points={
            'console_scripts': [
                f"{tool_id}-tool=nion.{tool_id}_tool.command:main",
            ],
        },
        distclass=BinaryDistribution,
        cmdclass={'bdist_wheel': bdist_wheel},
        verbose=True,
    )


# the bdist_wheel tools are awful and undocumented
# much of the techniques in this file were from other libraries and reading the source
# the wheel code is a separate project from setuptools

# see https://github.com/nion-software/nionui-launcher/releases
# see https://fredrikaverpil.github.io/2018/03/09/official-pyside2-wheels/
# see https://pypi.org/project/PySide2/#files
# see https://github.com/pypa/wheel
# see https://github.com/pypa/setuptools
# see https://github.com/pypa/wheel/issues/161
# see http://code.qt.io/cgit/pyside/pyside-setup.git/tree/build_scripts/wheel_override.py?id=824b7733c0bd8b162b198c67014d7f008fb71b8c

# everything will take place within pypi_build subdirectory
cwd = os.getcwd()
build_dir = pathlib.Path(cwd) / "pypi_build"
if os.path.exists(build_dir):
    shutil.rmtree(build_dir)
os.makedirs(build_dir)
os.chdir(build_dir)

# download the tool projects
download_url("https://github.com/nion-software/nionui-tool/archive/master.zip", "nionui-tool.zip")
download_url("https://github.com/nion-software/nionswift-tool/archive/master.zip", "nionswift-tool.zip")

# unzip the tool projects
subprocess.call(['unzip', '-q', "nionui-tool.zip", '-d', "nionui-tool"])
subprocess.call(['unzip', '-q', "nionswift-tool.zip", '-d', "nionswift-tool"])

# construct a nion subdirectory with the commands
os.makedirs("nion")
shutil.move("nionui-tool/nionui-tool-master/nion/nionui_tool", "nion/nionui_tool")
shutil.move("nionswift-tool/nionswift-tool-master/nion/nionswift_tool", "nion/nionswift_tool")

from multiprocessing import Process

# define the list of installs to build
# the fields are: tool_id, platform, python_version, abi, directory_path, install_path, project_id, version_tag
installs = [
    ("nionui", "macosx_10_11_intel", "cp35.cp36.cp37", "abi3", "NionUILauncher-Mac", "bin", "nionui-launcher", "v0.3.19"),
    ("nionui", "win_amd64", "cp35.cp36.cp37", "none", "NionUILauncher-Windows", "Scripts/NionUILauncher", "nionui-launcher", "v0.3.19"),
    ("nionui", "manylinux1_x86_64", "cp35.cp36.cp37", "abi3", "NionUILauncher-Linux", "bin/NionUILauncher", "nionui-launcher", "v0.3.19"),
    # ("nionswift", "macosx_10_11_intel", "cp35.cp36.cp37", "abi3", "NionSwiftLauncher-Mac", "bin", "nionswift-launcher", "0.3.17"),
    # ("nionswift", "win_amd64", "cp35.cp36.cp37", "none", "NionSwiftLauncher-Windows", "Scripts/NionSwiftLauncher", "nionswift-launcher", "0.3.17"),
    # ("nionswift", "manylinux1_x86_64", "cp35.cp36.cp37", "abi3", "NionSwiftLauncher-Linux", "bin/NionSwiftLauncher", "nionswift-launcher", "0.3.17"),
]

twines = list()

for install_args in installs:
    print("-------------------------------------------------------")
    # use multiprocessing because setup can only run once
    p = Process(target=setup, args=install_args)
    p.start()
    p.join()
    tool_id, platform_, python_version_, abi_, dir_path, dest, project, version = install_args
    version = re.sub('[^\d.]', '', version)
    old_file_path = f"dist/{tool_id}_tool-{version}-{python_version_}-{abi_}-{platform_}.whl"
    # hack for testing. this adjusts the version from 0.3.18 to 0.0.318 or something similar.
    # version = "0.0." + str(int(re.sub('[^\d]', '', version)) + 1)
    new_file_path = f"dist/{tool_id}_tool-{version}-{python_version_}-{abi_}-{platform_}.whl"
    # if version was modified, rename the file
    if old_file_path != new_file_path:
        shutil.move(old_file_path, new_file_path)
    # add the twines command to the list. printing at the end avoids interspersing extraneous output.
    twines.append(f"twine upload %s -r pypitest" % new_file_path)

print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++")
for twine in twines:
    print(twine)

# setup("macosx_10_11_intel", "cp35.cp36.cp37", "abi3", "NionUILauncher-Mac", "bin")

# setup("win_amd64", "cp35.cp36.cp37", "none", "NionUILauncher-Windows", "Scripts/NionUILauncher")

# setup("manylinux1_x86_64", "cp35.cp36.cp37", "abi3", "NionUILauncher-Linux", "bin/NionUILauncher")

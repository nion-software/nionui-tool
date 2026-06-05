import os
import pathlib
import platform
import setuptools
import sys
import sysconfig
import typing

tool_id = "nionui"
launcher = "NionUILauncher"

version = "5.3.0"


def package_files(directory: str, prefix: str, prefix_drop: int) -> list[typing.Tuple[str, list[str]]]:
    # note: Windows setup does not work with Path
    prefixes = dict[str, list[str]]()
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            full_path = pathlib.Path(path) / filename
            if not os.path.islink(str(full_path)):
                dest_path = pathlib.Path(prefix) / pathlib.Path(*pathlib.Path(path).parts[prefix_drop:])
                prefixes.setdefault(str(dest_path), list[str]()).append(str(pathlib.Path(path) / filename))
    return list(prefixes.items())


class BinaryDistribution(setuptools.Distribution):
    # force abi+platform in whl
    def has_data_files(self) -> bool:
        return True
    def has_ext_modules(self) -> bool:
        return True


import setuptools.command.bdist_wheel as bdist_wheel_
import packaging


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


# this class overrides some methods of bdist_wheel to avoid its stricter tag checks.
class bdist_wheel(bdist_wheel_.bdist_wheel):
    def get_tag(self) -> typing.Tuple[str, str, str]:
        global python_tag, abi_tag, platform_tag
        return python_tag, abi_tag, platform_tag


def is_arm64() -> bool:
    machine = platform.machine().lower()
    # 'aarch64' is common on Linux/Android
    # 'arm64' is common on macOS (Apple Silicon) and Windows
    return machine in ['arm64', 'aarch64']


python_tag = "cp312"  # minimum version
abi_tag = "abi3"
platform_tag = str()
dest = None
dir_path = None
dest_drop = None


if sys.platform == "darwin":
    platform_tag = sysconfig.get_platform().replace("-", "_").replace(".", "_")
    dest = "bin"
    dir_path = "launcher/build/Release"
    dest_drop = 3
if sys.platform == "win32":
    platform_tag = "win_amd64" if not is_arm64() else "win_arm64"
    dest = f"Scripts/{launcher}"
    dir_path = "launcher/x64/Release" if not is_arm64() else "launcher/arm64/Release"
    dest_drop = 3
if sys.platform == "linux":
    platform_tag = "manylinux_2_28_x86_64" if not is_arm64() else "manylinux_2_28_aarch64"
    dest = f"bin/{launcher}"
    dir_path = "launcher/linux/x64" if not is_arm64() else "launcher/linux/arm64"
    dest_drop = 3

data_files = package_files(dir_path, dest, dest_drop)


def long_description() -> str:
    with open('README.rst', 'r') as fi:
        result = fi.read()
    return result


setuptools.setup(
    name=f"{tool_id}-tool",
    version=version,
    packages=[f"nion.{tool_id}_tool"],
    url=f"https://github.com/nion-software/{tool_id}-tool",
    license='Apache-2.0',
    author='Nion Software Team',
    author_email='software@nion.com',
    description='Python command line access to Nion UI Launcher',
    long_description=long_description(),
    entry_points={
        'console_scripts': [
            f"{tool_id}-tool=nion.{tool_id}_tool.command:main",
        ],
    },
    data_files=data_files,
    distclass=BinaryDistribution,
    cmdclass={'bdist_wheel': bdist_wheel},
    install_requires=
    [
        'numpy >=2.0,<3.0'
    ],
    classifiers=[
        'License :: OSI Approved :: Apache Software License',
    ],
    verbose=True,
)

import os
import pathlib
import platform as platform_module
import setuptools
import sys
import typing

tool_id = "nionui"
launcher = "NionUILauncher"

version = "0.5.0"


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


from setuptools.command import bdist_wheel as bdist_wheel_
from packaging import tags


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
    def run(self) -> None:
        super().run()

    def finalize_options(self) -> None:
        super().finalize_options()
        self.universal = True
        self.plat_name_supplied = True
        global platform, python_version, abi
        self.plat_name = platform
        self.py_limited_api = python_version or str()
        self.abi_tag = abi

    def get_tag(self) -> typing.Tuple[str, str, str]:
        # bdist sets self.plat_name if unset, we should only use it for purepy
        # wheels if the user supplied it.
        if self.plat_name_supplied:
            plat_name = self.plat_name
        elif self.root_is_pure:
            plat_name = 'any'
        else:
            # macosx contains system version in platform name so need special handle
            if self.plat_name and not self.plat_name.startswith("macosx"):
                plat_name = self.plat_name
            else:
                plat_name = bdist_wheel_.get_platform(self.bdist_dir)

            if plat_name in ('linux-x86_64', 'linux_x86_64') and sys.maxsize == 2147483647:
                plat_name = 'linux_i686'
            else:
                plat_name = str()

        plat_name = plat_name.replace('-', '_').replace('.', '_')

        if self.root_is_pure:
            if self.universal:
                impl = 'py2.py3'
            else:
                impl = self.python_tag
            tag = (impl, 'none', plat_name)
        else:
            impl_name = tags.interpreter_name()
            impl_ver = tags.interpreter_version()
            impl = impl_name + impl_ver
            abi_tag = self.abi_tag
            tag = (impl, abi_tag, plat_name)
            supported_tags = [(t.interpreter, t.abi, t.platform) for t in tags.sys_tags()]
            # XXX switch to this alternate implementation for non-pure:
            if not self.py_limited_api:
                assert tag == supported_tags[0], "%s != %s" % (tag, supported_tags[0])
            # assert tag in supported_tags, "would build wheel with unsupported tag {}".format(tag)
        return tag


platform = None
python_version = None
abi = None
dest = None
dir_path = None
dest_drop = None

if sys.platform == "darwin":
    platform = "macosx_10_11_intel" if platform_module.processor() != "arm" else "macosx_11_0_arm64"
    python_version = "cp39.cp310.cp311.cp312"
    abi = "abi3"
    dest = "bin"
    dir_path = "launcher/build/Release"
    dest_drop = 3
if sys.platform == "win32":
    platform = "win_amd64"
    python_version = "cp39.cp310.cp311.cp312"
    abi = "none"
    dest = f"Scripts/{launcher}"
    dir_path = "launcher/x64/Release"
    dest_drop = 3
if sys.platform == "linux":
    platform = "manylinux1_x86_64"
    python_version = "cp39.cp310.cp311.cp312"
    abi = "abi3"
    dest = f"bin/{launcher}"
    dir_path = "launcher/linux/x64"
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
    classifiers=[
        'License :: OSI Approved :: Apache Software License',
    ],
    verbose=True,
)

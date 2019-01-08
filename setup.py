import os
import pathlib
import setuptools
import sys

tool_id = "nionui"
version = "0.3.21a8"
launcher = "NionUILauncher"


def package_files(directory, prefix, prefix_drop):
    # note: Windows setup does not work with Path
    prefixes = dict()
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            full_path = pathlib.Path(path) / filename
            if not os.path.islink(str(full_path)):
                dest_path = pathlib.Path(prefix) / pathlib.Path(*pathlib.Path(path).parts[prefix_drop:])
                prefixes.setdefault(str(dest_path), list()).append(str(pathlib.Path(path) / filename))
    return list(prefixes.items())


class BinaryDistribution(setuptools.Distribution):
    # force abi+platform in whl
    def has_data_files(self):
        return True
    def has_ext_modules(self):
        return True


from distutils.util import get_platform
from wheel.bdist_wheel import bdist_wheel as bdist_wheel_
from wheel.pep425tags import get_abbr_impl, get_impl_ver, get_abi_tag, get_platform, get_supported


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


platform = None
python_version = None
abi = None
dest = None
dir_path = None
dest_drop = None

if sys.platform == "darwin":
    platform = "macosx_10_11_intel"
    python_version = "cp36.cp37"
    abi = "abi3"
    dest = "bin"
    dir_path = "launcher/build/Release"
    dest_drop = 3
if sys.platform == "win32":
    platform = "win_amd64"
    python_version = "cp36.cp37"
    abi = "none"
    dest = f"Scripts/{launcher}"
    dir_path = "launcher/x64/Release"
    dest_drop = 3
if sys.platform == "linux":
    platform = "manylinux1_x86_64"
    python_version = "cp36.cp37"
    abi = "abi3"
    dest = f"bin/{launcher}"
    dir_path = "launcher/linux/x64"
    dest_drop = 3

data_files = package_files(dir_path, dest, dest_drop)

setuptools.setup(
    name=f"{tool_id}-tool",
    version=version,
    packages=[f"nion.{tool_id}_tool"],
    url='http://www.nion.com',
    license='Apache-2.0',
    author='Nion Software Team',
    author_email='software@nion.com',
    description='Python command line access to Nion UI Launcher',
    include_package_data=True,
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

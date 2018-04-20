# python setup.py bdist_wheel --plat-name macosx-10.7-x86_64

import glob
import os

from setuptools import setup

def package_files(directory):
    paths = []
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            paths.append(os.path.join('..', path, filename))
    return paths

extra_files = package_files('nui/macosx')

modules = ["nui"]

setup(
    name='nionui-tool',
    version='0.3.10',
    zip_safe=False,
    packages=modules,
    package_data={'': extra_files},
    url='http://www.nion.com',
    license='Apache 2.0',
    author='Nion Software Team',
    author_email='software@nion.com',
    description='Python command line access to Nion UI Launcher',
    include_package_data=True,
    entry_points={
        'console_scripts': [
            'nui=nui.command:main',
            ],
        },
)

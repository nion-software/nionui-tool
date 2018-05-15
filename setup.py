from setuptools import setup

setup(
    name='nionui-tool',
    version='0.3.14',
    packages=["nion.nionui_tool"],
    url='http://www.nion.com',
    license='Apache 2.0',
    author='Nion Software Team',
    author_email='software@nion.com',
    description='Python command line access to Nion UI Launcher',
    include_package_data=True,
    entry_points={
        'console_scripts': [
            'nionui-tool=nion.nionui_tool.command:main',
        ],
    },
)

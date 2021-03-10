import glob

from subprocess import call
from setuptools import setup, find_packages
from setuptools.command.install import install
from setuptools.command.develop import develop
from distutils.command.build import build

def generate_interfaces(path):
    call(['make'], cwd=path)

def compile_camera_server(path):
    call(['make'], cwd=path)

class Develop(develop):

    def run(self):
        develop.run(self)

        self.execute(generate_interfaces, ['.'], msg='Generating Thrift interfaces')
        self.execute(compile_camera_server, ['./server'], msg='Compiling camera server')

class Build(build):
    """Compiles the DetectorServer binary and generates Thrift interfaces."""

    def run(self):
        build.run(self)

        self.execute(generate_interfaces, ['.'], msg='Generating Thrift interfaces')
        self.execute(compile_camera_server, ['./server'], msg='Compiling camera server')

class Install(install):
    """Installs the DetectorServer binary."""

    def run(self):
        install.run(self)

        def _install(path):
            call(['make', 'install'], cwd=path)

        self.execute(
            _install, ['./server'], msg='Installing camera server')

setup(
    name="Mookodi",
    packages=find_packages(),
    install_requires=[
        'configparser==3.7.4',
    ],
    cmdclass={
        'develop': Develop,
        'build': Build,
        'install': Install,
    },
    data_files=[
    ],
    entry_points={
        'console_scripts': [
        ],
    }
)

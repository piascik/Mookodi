from setuptools import setup, find_packages
from lesedi.lib import __version__


setup(
    name='lesedi-lib',
    description='Lesedi Telescope Control Library',
    version=__version__,
    author='Carel van Gend',
    author_email='carel@saao.ac.za',
    license=open('LICENSE').read(),
    namespace_packages=['lesedi'],
    packages=find_packages(),
    install_requires=['thrift', 'astropy==3.2.3', 'pymodbus==2.3.0', 'Twisted==19.10.0'],
    data_files=[
        # System-wide configuration files.
        ('/etc/systemd/system/', [
            'etc/systemd/system/lesedi.service',
        ]),
        ('/etc/usr/local/etc/', [
            'etc/lesedi.ini',
        ]),
    ],
    entry_points={
        'console_scripts': [
            'lesedi = lesedi.lib.server:run',

            # These are purely for convenience when testing. The commands
            # launch a simulated device on the given port.
            'lesedi-aux = lesedi.lib.aux.driver:main',
            'lesedi-dome = lesedi.lib.dome.driver:main',
            'lesedi-focuser = lesedi.lib.focuser.driver:main',
            'lesedi-rotator = lesedi.lib.rotator.driver:main',
            'lesedi-telescope = lesedi.lib.telescope.driver:main',
        ],
    }
)

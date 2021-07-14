"""
mookodi-pipelines
Basic detector array reductions. Bias, Dark, Flat.
Wrappers to call ASPIRED for spectral reduction.
Functions for the astrometric analysis used by iterative acquistion.

Author
    Robert Smith (r.j.smith@ljmu.ac.uk)

#License
#   ????
March 2021
"""

__packagename__ = "mookodi-pipelines"

from setuptools import setup, find_packages

setup_requires = ['numpy']

install_requires=['numpy', 'astropy',
	'aspired @ git+https://github.com/cylammarco/ASPIRED.git@dev' ],

setup(name='mookodi-pipelines',
      version='0.0.1',
      author=['Robert Smith'],
      author_email=['r.j.smith@ljmu.ac.uk'],
      url="https://github.com/piascik/Mookodi",
      license='',
      ong_description=open('README.md').read(),
      packages=find_packages(),
      package_dir={},
      include_package_data=True,
      setup_requires=setup_requires,
      install_requires=install_requires,
      python_requires='>=2.7',
      entry_points={ })

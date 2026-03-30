from setuptools import find_packages
from setuptools import setup

setup(
    name='hal',
    version='0.0.1',
    packages=find_packages(
        include=('hal', 'hal.*')),
)

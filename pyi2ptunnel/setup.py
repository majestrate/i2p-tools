#!/usr/bin/env python
from setuptools import setup

setup(name = 'pyi2ptunnel',
    version = '0.0',
    description = 'i2p application tunnel server',
    author = 'Jeff Becker',
    author_email = 'ampernand@gmail.com',
    install_requires = ['txi2p', 'twisted'],
    tests_require=['pytest'],
    url = 'https://github.com/majestrate/i2p-tools',
    package_dir = {'': 'src'},
    packages = ['pyi2ptunnel'],
)

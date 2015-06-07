#!/usr/bin/env python
from setuptools import setup

setup(name = 'netdb',
    version = '0.0',
    description = 'i2p netdb parser',
    author = 'Jeff Becker',
    author_email = 'ampernand@gmail.com',
    install_requires = ['python-geoip','python-geoip-geolite2'],
    tests_require=['pytest'],
    url = 'https://github.com/majestrate/i2p-tools',
    package_dir = {'netdb': 'src'},
    packages = ['netdb'],
)

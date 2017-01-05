#!/usr/bin/env python
from setuptools import setup

setup(name = 'i2pcontrol',
    version = '0.2',
    description = 'i2pcontrol wrapper',
    author = 'Chris Barry',
    author_email = 'chris@barry.im',
	classifiers = [
	'Development Status :: 5 - Production/Stable',
	'License :: OSI Approved :: MIT License',
	'Topic :: Utilities',
	],
    install_requires = ['requests'], 
    tests_require=['pytest'],
    url = 'https://github.com/majestrate/i2p-tools',
    packages=['i2pcontrol']
)


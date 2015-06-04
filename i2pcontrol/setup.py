#!/usr/bin/env python
from setuptools import setup

setup(name = 'i2pcontrol',
    version = '0.1',
    description = 'i2pcontrol wrapper',
    author = 'Chris Barry',
    author_email = 'chris@barry.im',
	classifiers = 'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)',
    #install_requires = 'python-jsonrpc>=0.7.4',
    url = 'https://github.com/majestrate/i2p-tools',
    #package_dir = {'i2pcontrol': 'i2pcontrol'},
    packages = ['i2pcontrol', 'i2pcontrol.pyjsonrpc'],
)


from setuptools import setup
import os
import sys

long_description = ''
long_description_fname = 'README.rst'

if os.path.exists(long_description_fname):
    with open(long_description_fname, 'r') as infile:
        long_description = infile.read()


install_requires = []
install_requires_fname = 'requirements.txt'

if os.path.exists(install_requires_fname):
    with open(install_requires_fname, 'r') as infile:
        install_requires = infile.read().split()

if sys.version_info[0] < 3:
    install_requires.append("future>=0.14.0")

version = '0.0.1'

setup(
    name='i2tun',
    description='ip over i2p tunnel',
    long_description=long_description,
    author='Jeff',
    author_email='ampernand@gmail.com',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: Public Domain',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Topic :: Internet',
    ],
    license='Public Domain',
    version=version,
    install_requires=install_requires,
    packages=['i2p', 'i2p.socket', 'i2p.socket.sam', 'i2tun'],
)

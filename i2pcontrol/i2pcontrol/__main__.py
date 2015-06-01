#!/usr/bin/env python
# coding: utf-8
#
# __main__.py - Interact with an i2p node through json-rpc.
# Author: Chris Barry <chris@barry.im>
# License: This is free and unencumbered software released into the public domain.

from .i2pcontrol import *

# Little demo on how it could be used.
if __name__ == '__main__':
	a = I2PController()
	print(a.getNetworkSetting())
	vals = a.getRouterInfo()
	print(''.join([
		'You are running i2p version ', str(vals['i2p.router.version']), '. ',
		'It has been up for ', str(vals['i2p.router.uptime']), 'ms. ',
		'Your router knows ', str(vals['i2p.router.netdb.knownpeers']),' peers.'
		]))

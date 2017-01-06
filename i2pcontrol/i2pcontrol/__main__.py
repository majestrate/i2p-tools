#!/usr/bin/env python
# coding: utf-8
#
# __main__.py - Interact with an i2p node through json-rpc.
# Copyright (c) 2015 Chris Barry
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE

from .i2pcontrol import *
import os

# Little demo on how it could be used.
if __name__ == '__main__':
    home = os.environ["HOME"]
    cert = os.path.join(home, ".i2pd", "i2pcontrol.pem")
    if not os.path.exists(cert):
        cert = False
    a = I2PController(ssl_cert=cert)
    vals = a.getRouterInfo()
    print(''.join([
        'You are running i2p version ', str(vals['i2p.router.version']), '. ',
        'It has been up for ', str(vals['i2p.router.uptime']), 'ms. ',
        'Your router knows ', str(vals['i2p.router.netdb.knownpeers']),' peers.'
        ]))

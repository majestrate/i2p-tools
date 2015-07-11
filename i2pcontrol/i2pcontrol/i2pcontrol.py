#!/usr/bin/env python
# coding: utf-8
#
# i2pcontrol.py - Interact with an i2p node through json-rpc.
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

import pyjsonrpc, ssl

# This isn't really used right now.
class RouterStatus():
	OK = 0
	TESTING = 1 
	FIREWALLED = 2
	HIDDEN = 3
	WARN_FIREWALLED_AND_FAST = 4
	WARN_FIREWALLED_AND_FLOODFILL = 5
	WARN_FIREWALLED_WITH_INBOUND_TCP = 6
	WARN_FIREWALLED_WITH_UDP_DISABLED = 7
	ERROR_I2CP = 8
	ERROR_CLOCK_SKEW = 9
	ERROR_PRIVATE_TCP_ADDRESS = 10
	ERROR_SYMMETRIC_NAT = 11
	ERROR_UDP_PORT_IN_USE = 12
	ERROR_NO_ACTIVE_PEERS_CHECK_CONNECTION_AND_FIREWALL = 13
	ERROR_UDP_DISABLED_AND_TCP_UNSET = 14

# This isn't really used right now.
class I2PControlErrors():
	JSON_PARSE_ERRO = -32700
	INVALID_REQUEST = -32600
	METHOD_NOT_FOUND = -32601
	INVALID_PARAMETERS = -32602
	INTERNAL_ERROR = -32603
	# i2pcontrol specific
	INVALID_PASSWORD_PROVIDED = -32001
	NO_AUTH_TOKEN_PRESENTED = -32002
	AUTH_TOKEN_DOES_NOT_EXIST = -32003
	TOKEN_EXPIRED = -32004
	API_VERSION_NOT_SUPPLIED = -32005
	API_INCOMPATIBLE = -32006

# Documentation: http://i2p-projekt.i2p/en/docs/api/i2pcontrol
class I2PController:
	API_VERSION = 1
	DEFAULT_HOST = '127.0.0.1'
	DEFAULT_PORT = 7650
	DEFAULT_PASSWORD = 'itoopie'

	def __init__(self, address=(DEFAULT_HOST, DEFAULT_PORT), password=DEFAULT_PASSWORD, use_ssl=True):
		# I2PControl mandated SSL, even tho we will be localhost.
		# Since we don't know the cert, and it's local, let's ignore it.
		context = None
		proto = 'http'
		if use_ssl:
			context = ssl.create_default_context()
			context.check_hostname = False
			context.verify_mode = ssl.CERT_NONE
			proto += 's'
		self._password = password
		self._token = None
		self._http_client = pyjsonrpc.HttpClient(
			url = ''.join(['{}://'.format(proto),address[0],':',str(address[1]),'/']),
			ssl_context = context
		)

		result = self._http_client.call('Authenticate',{ 
			'API': I2PController.API_VERSION,
			'Password': self._password
			})

		# TODO: Deal with errors better.
		
		self._token = result['Token']

		if result['API'] != I2PController.API_VERSION:
			print('looks like the api\'s do not match')
	
	# Echo
	def echo(self, string='echo'):
		return self._http_client.call('Echo',{ 
				'Token': self._token,
				'Echo': string
				}
			)['Result']

	# GetRate
	# 300000 is 5 minutes, measured in ms.
	# http://i2p-projekt.i2p/en/misc/ratestats
	def getRate(self, stat='', period=5*60*1000):
		return self._http_client.call('GetRate', {
				'Token': self._token,
				'Period': period,
				'Stat': stat
				}
			)['Result']

	# I2PControl
	def getI2PControlSettings(self, address=DEFAULT_HOST, password=DEFAULT_PASSWORD, port=DEFAULT_PORT):
		return self._http_client.call('I2PControl', {
				'Token': self._token,
				'SettingsSaved': '',
				'RestartNeeded': ''
				})

	def setI2PControlSettings(self, address=DEFAULT_HOST, password=DEFAULT_PASSWORD, port=DEFAULT_PORT):
		result = self._http_client.call('I2PControl', {
				'Token': self._token,
				'i2pcontrol.address': address,
				'i2pcontrol.port': port,
				'i2pcontrol.password': password,
				})
		# TODO: Update class variables
		return result
	
	# RouterInfo
	def getRouterInfo(self):
		return self._http_client.call('RouterInfo', {
				'Token': self._token,
				'i2p.router.status':'',
				'i2p.router.uptime':'',
				'i2p.router.version':'',
				'i2p.router.net.bw.inbound.1s':'',
				'i2p.router.net.bw.inbound.15s':'',
				'i2p.router.net.bw.outbound.1s':'',
				'i2p.router.net.bw.outbound.15s':'',
				'i2p.router.net.status':'',
				'i2p.router.net.tunnels.participating':'',
				'i2p.router.netdb.activepeers':'',
				'i2p.router.netdb.fastpeers':'',
				'i2p.router.netdb.highcapacitypeers':'',
				'i2p.router.netdb.isreseeding':'',
				'i2p.router.netdb.knownpeers':''
				})
	
	# RouterManager
	def reseed(self):
		return self._http_client.call('RouterManager', {
				'Token': self._token,
				'Reseed': ''
				})

	def restart(self):
		return self._http_client.call('RouterManager', {
				'Token': self._token,
				'Restart': ''
				})

	def restart_graceful(self):
		return self._http_client.call('RouterManager', {
				'Token': self._token,
				'RestartGraceful': ''
				})

	def shutdown(self):
		return self._http_client.call('RouterManager', {
				'Token': self._token,
				'Shutdown': ''
				})

	def shutdown_graceful(self):
		return self._http_client.call('RouterManager', {
				'Token': self._token,
				'ShutdownGraceful': ''
				})
	
	# NetworkSettings
	def setNetworkSetting(self, setting='', value=''):
		return self._http_client.call('NetworkSetting', {
				'Token': self._token,
				setting: value
				})
		
	def getNetworkSetting(self):
		return self._http_client.call('NetworkSetting', {
				'Token': self._token,
				'i2p.router.net.ntcp.port': None,
				'i2p.router.net.ntcp.hostname': None,
				'i2p.router.net.ntcp.autoip': None,
				'i2p.router.net.ssu.port': None,
				'i2p.router.net.ssu.hostname': None,
				'i2p.router.net.ssu.autoip': None,
				'i2p.router.net.ssu.detectedip': None,
				'i2p.router.net.upnp': None,
				'i2p.router.net.bw.share': None,
				'i2p.router.net.bw.in': None,
				'i2p.router.net.bw.out': None,
				'i2p.router.net.laptopmode': None
				})


from __future__ import absolute_import, division
from twisted.python.compat import urllib_parse, urlquote
from twisted.internet.protocol import ClientFactory
from twisted.internet.endpoints import connectProtocol
from twisted.web.http import HTTPFactory, HTTPClient, HTTPChannel, Request
from twisted.web.proxy import ProxyClient, ProxyClientFactory
from twisted.internet import reactor
from pyi2ptunnel import util


class _I2PProxyRequest(Request):

    protocols = {
        b'http' : ProxyClient,
    }
    ports = {
        b'http' : 80,
    }
    
    def __init__(self, channel, queued, reactor=reactor):
        Request.__init__(self, channel, queued)
        self.reactor = reactor
        self.endpointFactory = channel.createEndpoint

    def process(self):
        parsed = urllib_parse.urlparse(self.uri)
        protocol = parsed[0]
        host = parsed[1].decode('ascii')
        if protocol in self.ports:
            port = self.ports[protocol]
        else:
            # handle
            pass

        if ':' in host:
            host, port = host.split(':')
            port = int(port)
        rest = urllib_parse.urlunparse((b'', b'') + parsed[2:])
        if not rest:
            rest = rest + b'/'
        factory = self.protocols[protocol]
        headers = self.getAllHeaders().copy()
        if b'host' not in headers:
            headers[b'host'] = host.encode('ascii')
            
        headers[b'User-Agent'] = b'I2P'
            
        self.content.seek(0, 0)
        s = self.content.read()
        client = factory(self.method, rest, self.clientproto, headers, s, self)
        ep = self.endpointFactory(host, port)
        connectProtocol(ep, client)
        
class _Proxy(HTTPChannel):

    requestFactory = _I2PProxyRequest


def Proxy(createEndpoint, **kwargs):
    factory = _Proxy
    factory.createEndpoint = createEndpoint
    return factory

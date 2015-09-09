__doc__ = """
application layer filtering i2p client/server tunnels
"""


import logging

from pyi2ptunnel import tunnels

from twisted.internet import reactor
from twisted.internet.endpoints import clientFromString

log = logging.getLogger("pyi2ptunnel.tunnel")


class TunnelFactory(object):
    """
    creator of app tunnels
    """

    def __init__(self, api="SAM", apiEndpoint="127.0.0.1:7656", socksPort=9050):
        self.api = api
        self.apiEndpoint = apiEndpoint
        self.socksPort = socksPort

    def endpoint(self, addr, port): 
        if addr.endswith('.i2p'):
            ep = 'i2p:{}:api={}'.format(addr, self.api)
        else:
            ep = 'tor:host={}:port={}:socksPort={}'.format(addr, port, socksPort)
        print (ep)
        return clientFromString(reactor, ep)
        
                                
                                    
        
    def createClient(self, type, **param):
        """
        :returns: a twisted protocol factory
        """
        if type in tunnels.clients:
            return tunnels.clients[type](self.endpoint, **param)
        
    def createServer(self, type, **param):
        if type in tunnels.servers:
            return tunnels.servers[type](self.endpoint, **param)

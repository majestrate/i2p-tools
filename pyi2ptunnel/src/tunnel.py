__doc__ = """
application layer filtering i2p client/server tunnels
"""


import logging

from . import tunnels

log = logging.getLogger("pyi2ptunnel.tunnel")


class TunnelFactory(object):
    """
    creator of app tunnels
    """

    def __init__(self, api="SAM", apiEndpoint="127.0.0.1:7656"):
        self._api = api
        self._apiEndpoint = apiEndpoint


    def create(self, type, **param):
        """
        :returns: a twisted protocol factory
        """
        if type in tunnels.types:
            return tunnels.types[type](**param)
        

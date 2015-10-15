__doc__ = """
i2p aware application layer filtering proxy tunnels
"""

from . import http

clients = {
    "http" : http.Proxy
}

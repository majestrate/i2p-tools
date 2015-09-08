__doc__ = """
i2p aware application layer filtering proxy tunnels
"""

from . import http

types = {
    "http-client" : http.I2PProxy
}

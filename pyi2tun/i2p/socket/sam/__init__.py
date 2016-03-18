__doc__ = """
sam3 backend to i2p.socket
"""

from i2p.socket.sam import simple
import socket as pysocket

SOCK_STREAM = pysocket.SOCK_STREAM
SOCK_DGRAM = pysocket.SOCK_DGRAM
SOCK_RAW = pysocket.SOCK_RAW

# socket flags for shutdown()
SHUT_RD = pysocket.SHUT_RD
SHUT_WR = pysocket.SHUT_WR
SHUT_RDWR = pysocket.SHUT_RDWR

# Address family for i2p
# what! 9000?!
AF_I2P = 9002

# pysocket address families
AF_INET = pysocket.AF_INET
AF_INET6 = pysocket.AF_INET6

# TODO: socket options

class i2p_socket:

    def __init__(self, type, samaddr, dgramaddr, dgrambind):
        """
        create an i2p socket
        """
        if type == SOCK_STREAM:
            type = simple.SAM.SOCK_STREAM
        elif type == SOCK_DGRAM:
            type = simple.SAM.SOCK_DGRAM
        elif type == SOCK_RAW:
            type = simple.SAM.SOCK_RAW
        else:
            raise ValueError("invalid socket type with AF_I2P")
        sock = simple.Socket(samaddr, dgramaddr, dgrambind, type)
        self.recv = sock.recv
        self.close = sock.close
        self.bind = sock.bind
        self.send = sock.send
        self.connect = sock.connect
        self.sendto = sock.sendto
        self.recvfrom = sock.recvfrom
        self.fileno = sock.fileno
        self.getsocketinfo = sock.getsocketinfo

def socket(family=AF_INET, type=SOCK_STREAM, proto=0, samaddr=('127.0.0.1', 7656), dgramaddr=('127.0.0.1', 7655), dgrambind=('127.0.0.1', 0)):
    """
    wraps socket.socket
    if family is AF_I2P, the socket will use i2p otherwise it will call socket.socket
    """
    if family == AF_I2P:
        return i2p_socket(type, samaddr, dgramaddr, dgrambind)
    else:
        return pysocket.socket(family, type, proto)
        
def create_connection(address, timeout=60, source_address=None):
    s = socket()
    s.connect(address)
    return s

def getaddrinfo(host, *args, **kwargs):
    if host.endswith(".i2p"):
        return [(AF_SAM, SOCK_STREAM, 0, host, 0)]
    return pysocket.getaddrinfo(host, *args, **kwargs)

__doc__ = """
monkey patch another module to use i2p.socket
"""


def patch(module, new_name="_py_socket"):
    """
    monkey patch all references of socket to use i2p.socket
    """

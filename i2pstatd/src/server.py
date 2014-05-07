__doc__ = """
stats collection server
"""


class NDBStatServer:

    def __init__(self, ndb_dir):
        self._ndb_dir = ndb_dir


    def inspect(self):
        """
        inspect netdb
        """

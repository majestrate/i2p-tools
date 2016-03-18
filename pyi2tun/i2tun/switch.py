
class Switch:
    """
    handler of packets
    """

    def __init__(self, tundev):
        self._tundev = tundev
        self._dests = dict()
        self._addrs = dict()
        self._frameno = 0
        self._rpc = None
        

    def getOurIP(self):
        """
        return our ip address as string
        """
        return self._tundev.addr

    def isServer(self):
        """
        return true if we are operating in server mode
        """
        
    def registerAddr(self, dest, addr):
        self._dests[addr] = dest
        self._addrs[dest] = addr

    def addrForDest(self, dest):
        """
        given a destination get their address
        """
        if dest in self._addrs:
            return self._addrs[dest]

    def destForAddr(self, addr):
        """
        get the destination for this ip address
        :param addr: address to look for
        :return: None when not found or a desthash
        """
        if addr in self._dests:
            return self._dests[addr]
    
    def next_frame(self):
        self._frameno += 1
        return self._frameno

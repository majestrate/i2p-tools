
from i2p import socket
from i2p.datatypes import Destination
import asyncio
import collections
import operator
import logging

class SAMLink:

    _log = logging.getLogger("i2p.tun.Handler")

    def __init__(self, netif, tap, switch, protocol, keyfile, samcfg, loop=None):
        """
        """
        self._pump_interval = protocol.pumpInterval
        self.switch = switch
        if tap:
            self._frame_type = protocol.FrameType.ETHER
        else:
            self._frame_type = protocol.FrameType.IP
        self._netif = netif
        self._protocol = protocol
        self._write_buff = collections.deque() 
        self._read_buff = collections.deque()
        self.loop = loop or asyncio.get_event_loop()
        self._bw = 0
        self._pps = 0
        self._log.debug("creating sam")

        samaddr = (samcfg["controlHost"], samcfg["controlPort"])
        dgramaddr = (samcfg["dgramHost"], samcfg["dgramPort"])
        dgrambind = (samcfg["dgramBind"], 0)
        self._conn = socket.socket(socket.AF_I2P, type=socket.SOCK_DGRAM, samaddr=samaddr, dgramaddr=dgramaddr, dgrambind=dgrambind)
        self._conn.bind(keyfile, **samcfg["opts"])
        self._log.debug("sam bound")
        self.dest = self._conn.getsocketinfo()
        self.loop.add_reader(self._netif, self._read_netif)
        self.loop.add_reader(self._conn, self._read_sock)
        self.loop.call_soon(self._pump)
        
    def send_dgram(self, dest, data):
        """
        schedule a packet of data to dest
        """
        self._write_buff.append((dest,data))

    def get_status(self):
        """
        :return: r, w
        """
        return len(self._read_buff), len(self._write_buff)
    
    def _pump(self):

        # create frame to send to remote
        if len(self._read_buff) > 0:
            # group packets 
            pkts = dict()
            while len(self._read_buff) > 0:
                addr, buff = self._read_buff.pop()
                if addr not in pkts:
                    pkts[addr] = collections.deque()
                pkts[addr].append(buff)

            # make frames
            for addr, buff, in pkts.items():
                # create frames
                frames = self._protocol.createFrames(buff, self._frame_type)
                # get dest for ip
                dest = self._get_dest_for_addr(addr)
                if dest is None:
                    self._log.warn("unknown dest for addr: {}".format(addr))
                else:
                    # send frames
                    for f in frames:
                        self._send_frame(dest, f.data)
        frames = list()
        # get all frames from remote
        while len(self._write_buff) > 0:
            # read frame from remote
            dest, data = self._write_buff.pop()
            f = self._protocol.parseFrame(data, dest)
            if f:
                for frame in f:
                    frames.append(frame)

        # sort in place by frame number
        frames.sort(key=lambda f : f.frameno)
        # pump frames
        self._pump_frames(frames)
        # call again
        self.loop.call_later(self._pump_interval, self._pump)

            
    def _pump_frames(self, frames):
        for f in frames:
            if f.type == self._protocol.FrameType.ETHER and self._use_ether:
                self.loop.call_soon(self._netif.write, f.data)
            elif f.type == self._protocol.FrameType.IP:
                self.loop.call_soon(self._netif.write, f.data)
            elif f.type == self._protocol.FrameType.Control:
                # handle a control message
                self._handle_control(f.dest, f.data)
            elif f.type == self._protocol.FrameType.KeepAlive:
                # keep this guy alive
                self._keep_alive(f.dest)
            elif f.type == self._protocol.FrameType.ACK:
                # we got a packet of just tcp acknolegements
                self.loop.call_soon(self._netif.write, f.data)
            else:
                self._log.warn('invalid packet type in frame: {}'.format(f.type))


    def _handle_control(self, dest, data):
        """
        handle control packet
        :param dest: source destination
        :param data: bencoded data
        """
        d = self.switch
        
        
    def _read_netif(self):
        """
        read from network interface
        queue packets to remote endpoint sender
        """
        # read packet + overhead
        self._log.debug('readif')
        buff = self._netif.read(self._protocol.mtu + 64)
        addr = '%d.%d.%d.%d' % ( buff[20], buff[21], buff[22], buff[23]) 
        self._read_buff.append((addr, buff))

    def _read_sock(self):
        self._log.debug('read sock')
        result = self._conn.recvfrom(self._protocol.mtu + 64)
        if result:
            dest, pkt = result
            self._write_buff.append((dest, pkt))
            
    def _send_frame(self, dest, data):
        """
        send a frame to remote endpoint
        """
        self._log.debug("write {} to {}".format(len(data), dest))        
        # send to endpoint
        self._conn.sendto(data, (dest,0))

    def _get_dest_for_addr(self, addr):
        return self.switch.destForAddr(addr)


Handler = SAMLink

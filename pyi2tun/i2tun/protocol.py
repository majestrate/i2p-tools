from enum import Enum
import collections
import struct

ClumpingPacket = collections.namedtuple("ClumpingPacket", ["type", "data", "frameno", "dest"])

class ClumpingFrameType(Enum):
    """
    frame types for clumping frames
    """
    KeepAlive = 1 << 0
    IP = 1 << 1
    Control = 1 << 2
    ACK = 1 << 3
    ETHER = 1 << 4
    
class Clumping:
    """
    clumping frame protocol
    clumps messages into a single link level message
    """

    FrameType = ClumpingFrameType
    Packet = ClumpingPacket
    # 12 byte overhead per frame
    # 2 bytes type, 2 bytes length, 8 bytes frame number
    _frame_overhead = 12
    # 2 byte overhead per packet
    _packet_overhead = 2
    
    def __init__(self, mtu, frameno_func, pumpInterval):
        self.mtu = int(mtu)
        self.next_frameno = frameno_func
        self.pumpInterval = pumpInterval
    
    def parseFrame(self, data, dest):
        """
        :param data: bytearray
        :returns a generator of all packets:
        """
        type, packets, frameno = struct.unpack('>HHQ', data[:self._frame_overhead])
        data = data[self._frame_overhead:]
        if packets > 0:
            for _ in range(packets):
                if len(data) > 2:
                    l = struct.unpack('>H', data[:2])[0]
                    yield self.Packet(self.FrameType(type), data[2:2+l], frameno, dest)
                    data = data[2+l:]

    def _create_frame(self, packets, type):
        """
        create 1 frame
        the total size of the frame must fit the link mtu
        """
        fr = bytearray()
        fr += struct.pack('>H', type.value)
        fr += struct.pack('>H', len(packets))
        frameno = self.next_frameno()
        fr += struct.pack('>Q', frameno)
        for pkt in packets:
            fr += struct.pack('>H', len(pkt))
            fr += pkt
        return self.Packet(type, fr, frameno, None)

    def createFrames(self, packets, type):
        """
        :param packets: a list of Packet instances
        :param type: link level frame type
        :returns a generator yielding each frame created:
        """
        if len(packets) > 0:
            current_frame_packets = list()
            current_frame_size = self._frame_overhead
            for pkt in packets:
                if current_frame_size + len(pkt) < self.mtu:
                    current_frame_packets.append(pkt)
                    current_frame_size += len(pkt) + self._packet_overhead
                else:
                    yield self._create_frame(current_frame_packets, type)
                    current_frame_packets = list()
            yield self._create_frame(current_frame_packets, type)

class Flat(Clumping):
    """
    subset of the clumping protocol that doesn't actually clump
    """

    def createFrames(self, packets, type):
        for pkt in packets:
            yield self._create_frame([pkt], type)
    
            
class BencodeRPC:
    """
    bencoded rpc protocol
    """

    

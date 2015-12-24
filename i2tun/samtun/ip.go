package samtun
import (
  "net"
)

type ipPacket []byte

func (pkt ipPacket) setDst(addr net.IP) {
  pkt[16] = addr[0]
  pkt[17] = addr[1]
  pkt[18] = addr[2]
  pkt[19] = addr[3]
}

func (pkt ipPacket) setSrc(addr net.IP) {
  pkt[12] = addr[0]
  pkt[13] = addr[1]
  pkt[14] = addr[2]
  pkt[15] = addr[3]  
}

func (pkt ipPacket) Dst() net.IP {
  return net.IPv4(pkt[16], pkt[17], pkt[18], pkt[19])
}

func (pkt ipPacket) Src() net.IP {
  return net.IPv4(pkt[12], pkt[13], pkt[14], pkt[15])
}

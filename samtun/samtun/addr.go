//
// addr.go -- address tools
//
package samtun

import (
  "bitbucket.org/majestrate/sam3"
  "log"
  "net"
)
// maps b32 -> ip
type addrMap map[string]string

// given ip get b32
func (m addrMap) IP(b32 string) (ip string) {
  ip, _ = m[b32]
  return
}

// given b32 get ip
func (m addrMap) B32(ip string) string {
  for k, v := range m {
    if v == ip {
      return k
    }
  }
  return ""
}

// take a link message and filter the packets
// return a link frame that has corrected addresses
// returns nil if we have a packet from someone unmapped
func (m addrMap) filterMessage(msg linkMessage, ourAddr sam3.I2PAddr) (pkt ipPacket) {
  dst := net.ParseIP(m.IP(ourAddr.Base32()))
  src := net.ParseIP(m.IP(msg.addr.Base32()))
  if dst == nil || src == nil {
    // bad address
    pkt = nil
  } else {
    pkt = msg.pkt
    if pkt == nil || len(pkt) < 20 {
      // back packet
      log.Println("short packet from", src, len(pkt), "bytes")
      pkt = nil
    } else {
      pkt.setDst(dst)
      pkt.setSrc(src)
    }
  }
  return
}

//
// addr.go -- address tools
//
package samtun

import (
  "github.com/majestrate/i2p-tools/sam3"
  "log"
  "net"
)
// maps b32 -> ip range
type addrMap map[string]string

// given b32 get ip range
func (m addrMap) IP(b32 string) (ip *net.IPNet) {
  ipstr, _ := m[b32]
  if len(ipstr) > 0 {
    _, ip, _ = net.ParseCIDR(ipstr)
  }
  return
}

// given ip get b32
func (m addrMap) B32(ip string) string {
  i := net.ParseIP(ip)
  for k, v := range m {
    _, cidr, err := net.ParseCIDR(v)
    if err == nil && cidr.Contains(i) {
      return k
    }
  }
  return ""
}

// take a link message and filter the packets
// return a link frame that has corrected addresses
// returns nil if we have a packet from someone unmapped
func (m addrMap) filterMessage(msg linkMessage, ourAddr sam3.I2PAddr) (pkt ipPacket) {
  dst := m.IP(ourAddr.Base32())
  src := m.IP(msg.addr.Base32())
  if dst == nil || src == nil {
    // bad address
    return
  } else {

    if msg.pkt == nil || len(msg.pkt) < 20 {
      // back packet
      log.Println("short packet from", src, len(pkt), "bytes")
      return
    } else {
      if dst.Contains(msg.pkt.Dst()) && src.Contains(msg.pkt.Src()) {
        return msg.pkt
      }
    }
  }
  return
}

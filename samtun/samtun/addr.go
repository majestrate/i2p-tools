//
// addr.go -- address tools
//
package samtun

import (
  "bytes"
  "encoding/json"
  "github.com/majestrate/i2p-tools/sam3"
  "log"
  "io/ioutil"
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

func saveAddrMap(fname string, amap addrMap) (err error) {
  var data []byte
  data, err = json.Marshal(amap)
  if err == nil {
    var buff bytes.Buffer
    err = json.Indent(&buff, data, " ", "  ")
    if err == nil {
      err = ioutil.WriteFile(fname, buff.Bytes(), 0600)
    }
  }
  return
}

func loadAddrMap(fname string) (amap addrMap, err error) {
  var data []byte
  data, err = ioutil.ReadFile(fname)
  if err == nil {
    amap = make(addrMap)
    err = json.Unmarshal(data, &amap)
  }
  return
}

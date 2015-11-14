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
// maps b32 -> list of ip ranges
type addrMap map[string][]string

type addrList []*net.IPNet

// given ip get b32
func (m addrMap) B32(ip string) string {
  i := net.ParseIP(ip)
  for k, ranges := range m {
    for _, r := range ranges {
      _, cidr, err := net.ParseCIDR(r)
      if err == nil && cidr.Contains(i) {
        return k
      }
    }
  }
  return ""
}

// is this base32 mapped?
func (m addrMap) AllowDest(b32 string) (ok bool) {
  _, ok = m[b32]
  return
}

// get the ip ranges allowed on a b32
func (m addrMap) Ranges(b32 string) (nets addrList) {
  ranges, ok := m[b32]
  if ok {
    for _, r := range ranges {
      _, cidr, err := net.ParseCIDR(r)
      if err == nil {
        nets = append(nets, cidr)
      }
    }
  } else {
    nets = nil
  }
  return
}

// does this address list allow an ip address
func (m addrList) Contains(addr net.IP) bool {
  for _, n := range m {
    if n.Contains(addr) {
      return true
    }
  }
  return false
}

// take a link message and filter the packets
// return a link frame that has corrected addresses
// returns nil if we have a packet from someone unmapped
func (m addrMap) filterMessage(msg linkMessage, ourAddr sam3.I2PAddr) (pkt ipPacket) {
  dst := m.Ranges(ourAddr.Base32())
  src := m.Ranges(msg.addr.Base32())
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

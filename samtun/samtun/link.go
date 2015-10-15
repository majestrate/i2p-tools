//
// link.go -- link level protocol
//
package samtun

import (
  "bitbucket.org/majestrate/sam3"
  "bytes"
  "encoding/binary"
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

// a link level message
// infact a bunch of ip packets
type linkFrame []ipPacket

// get a raw bytes representation of this frame
func (f linkFrame) Bytes() []byte {
  var buff bytes.Buffer
  // link frame protocol version
  buff.Write([]byte{0})
  // link frame packet count
  pkts := len(f)
  buff.Write([]byte{byte(pkts)})
  // for each packet
  for _, pkt := range f {
    // pack the packet int our buffer
    pktbuff := make([]byte, len(pkt) + 2)
    binary.BigEndian.PutUint16(pktbuff[:], uint16(len(pkt)))
    copy(pktbuff[2:], pkt[:])
    buff.Write(pktbuff)
  }
  return buff.Bytes()
}

// create a linkFrame from a byteslice
// returns nil if format invalid
func frameFromBytes(buff []byte) (f linkFrame) {
  if buff[0] == 0x00 {
    pkts := buff[1]
    idx := 0
    for pkts > 0 {
      plen := binary.BigEndian.Uint16(buff[:2+idx])
      if 2 + int(plen) + idx < len(buff) {
        pkt := make([]byte, int(plen))
        copy(pkt, buff[idx+2:2+len(pkt)+idx])
        f = append(f, ipPacket(pkt))
        pkts--
        idx += len(pkt) + 2
      } else {
        return nil
      }
    }
  }
  return
}

// a link layer message that is sent over i2p
type linkMessage struct {
  frame linkFrame
  addr sam3.I2PAddr
}


func (m *linkMessage) appendPacket(pkt ipPacket) {
  m.frame = append(m.frame, pkt)
}

// get raw bytes representation to send over the wire
func (m *linkMessage) WireBytes() []byte {
  return m.frame.Bytes()
}

type linkProtocol struct {
  // key name for link message data
  msgdata string
  // key name for link protocol version number
  version string
  // key name for link message type
  msgtype string
}

// given a byte slice, read out a linkMessage
//func (p linkProtocol) Parse(data []byte) (pkts []packet, err error) {
//  bencode.
//}

var link = linkProtocol{
  msgdata: "x",
  msgtype: "w",
  version: "v",
}


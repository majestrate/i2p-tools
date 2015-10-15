//
// link.go -- link level protocol
//
package samtun

import (
  //bencode "github.com/majestrate/bencode-go"
  "bitbucket.org/majestrate/sam3"
)

// network level packet, usually an ipv4 packet
type packet []byte

// a link message that is sent over i2p
// contains many packets
type linkMessage struct {
  pkts []packet
  addr sam3.I2PAddr
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


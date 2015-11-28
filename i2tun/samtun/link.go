//
// link.go -- link level protocol
//
package samtun

import (
  i2p "github.com/majestrate/i2p-tools/sam3"
  "bytes"
  "encoding/binary"
  "errors"
  "io"
)

const (
  NULL_METH = iota
  /*
  client / server says that we are alive
  param: 'pn' -> packet number for jitter calculation
  return: no response needed
  */
  ALIVE_PING
  /**
  client says make new tunnel to inet address range
  param: 'cidr' -> the cidr to access
  return: tunnel id or 0 if rejected
  */
  CLIENT_TUN_NEW
  /**
  client says done with tunnel
  param: 'tid' -> the tunnel id or 0 for all
  return: always 0
  */
  CLIENT_TUN_DEL
  /**
  server says tunnel with tunnel id that we had is now gone
  param: 'tid' -> the tunnel who is gone
  return: always the value of the param 'tid' 
  */
  SERVER_TUN_BAI
  /*
  either client or server says "here are ip packets for tunnel with tid"
  param: 'tid' -> the tunnel that we are sending on
  param: 'pkts' -> bytes of link packets
  return: no response needed
  */
  RELAY_TUN_DATA 
)

var K_TID = "tid"
var K_PKTS = "pkts"
var K_CIDR = "cidr"
var K_PN = "pn"

// a link level message
type linkFrame struct {
  method int64 // method number
  param map[string]interface{} // parameters
  response int64 // response code
}

// encode this link frame to a writer
func (f *linkFrame) Encode(w io.Writer) (err error) {
  i := make(map[string]interface{})
  i["m"] = f.method
  i["p"] = f.param
  if f.response < 0 {
    // not set
  } else {
    i["r"] = f.response
  }
  return BencodeWriteMap(i, w)
}

// decode from reader into this frame
func (f *linkFrame) Decode(r io.Reader) (err error) {
  var m map[string]interface{}
  m, err = BencodeReadMap(r)
  if err == nil {
    v, ok := m["m"]
    if ok {
      switch v.(type) {
      case int64:
        f.method = v.(int64)
        break
      default:
        err = errors.New("bad method type")
        return
      }
    } else {
      // no method?
      err = errors.New("no method specified")
      return
    }
    v, ok = m["r"]
    if ok {
      switch v.(type) {
      case int64:
        f.response = v.(int64)
        break
      default:
        err = errors.New("bad response type")
        return
      }
    }
    v, ok = m["p"]
    if ok {
      switch v.(type) {
      case map[string]interface{}:
        f.param = v.(map[string]interface{})
        break
      default:
        err = errors.New("Bad parameters type")
        return
      }
    } else {
      // no parameters?
      err = errors.New("no parameters")
      return
    }
  }
  return
}

// get parameter as byteslice
func (f *linkFrame) GetParamStr(k string) (str []byte, err error) {
  v, ok := f.param[k]
  if ok {
    // we got it
    var s []byte
    switch v.(type) {
    case []byte:
      s = v.([]byte)
      str = make([]byte, len(s))
      copy(str, s)
      break
    default:
      err = errors.New("param value "+k+" not bytes")
    }
  } else {
    err = errors.New("params don't have "+k)
  }
  return
}

// get parameter as int
func (f *linkFrame) GetParamInt(k string) (i int64, err error) {
  v, ok := f.param[k]
  if ok {
    switch v.(type) {
    case int64:
      i = v.(int64)
      break
    default:
      err = errors.New("param value "+k+" not int")
    }
  } else {
    err = errors.New("params don't have "+k)
  }
  return
}

// does this link frame have ip packets?
func (f *linkFrame) HasPackets() (has bool) {
  if f.method == RELAY_TUN_DATA {
    _, has = f.param[K_PKTS]
  }
  return
}

func bytesFromPkts(pkts []ipPacket) []byte {
  var buff bytes.Buffer
  // packet format
  buff.Write([]byte{0})
  // packet count
  n := len(pkts)
  buff.Write([]byte{byte(n)})
  // for each packet
  for _, pkt := range pkts {
    // pack the packet int our buffer
    pktbuff := make([]byte, len(pkt) + 2)
    binary.BigEndian.PutUint16(pktbuff, uint16(len(pkt)))
    copy(pktbuff[2:], pkt[:])
    buff.Write(pktbuff)
  }
  return buff.Bytes()
}

// reads packets from byteslice
// returns nil if format invalid
func pktsFromBytes(buff []byte) (pkts []ipPacket) {
  if buff[0] == 0x00 {
    n := buff[1]
    idx := 0
    for n > 0 {
      plen := binary.BigEndian.Uint16(buff[:2+idx])
      if 2 + int(plen) + idx < len(buff) {
        pkt := make([]byte, int(plen))
        copy(pkt, buff[idx+2:2+len(pkt)+idx])
        pkts = append(pkts, ipPacket(pkt))
        n--
        idx += len(pkt) + 2
      } else {
        return nil
      }
    }
  }
  return
}


func newFrameFromPackets(pkts []ipPacket, tid int64) (f linkFrame) {
  f.method = RELAY_TUN_DATA
  f.param = map[string]interface{} { K_PKTS : bytesFromPkts(pkts), K_TID: tid}
  f.response = -1
  return
}

// a link layer message that is sent over i2p
type linkMessage struct {
  frame linkFrame
  addr i2p.I2PAddr
}

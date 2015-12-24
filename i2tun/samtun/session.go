package samtun
//
//
//

import (
  i2p "github.com/majestrate/i2p-tools/sam3"
  "log"
  "net"
)

type linkTunnel struct {
  tid int64 // tunnel id
  allowed *net.IPNet // allowed destination ip range
}

type linkSession struct {
  remote i2p.I2PAddr // session's destination
  addr net.IP // session's IP
  tunnels map[int64]*linkTunnel // all active tunnels
  acceptTunnel func(*net.IPNet)
}

func (s *linkSession) addTunnel(t *linkTunnel) {
  if t != nil {
    _, has := s.tunnels[t.tid]
    if has {
      // error?
    } else {
      s.tunnels[t.tid] = t
    }
  }
}

// get the tunnel id for a packet that we got from inet land
// returns 0 if not found
func (s *linkSession) TunIdForPkt(pkt ipPacket) (tid int64) {
  addr := pkt.Src()
  for _, tun := range s.tunnels {
    if tun.allowed.Contains(addr) {
      tid = tun.tid
      break
    }
  }
  return
}

// we got a packet from inet land :D
func (s *linkSession) gotInetPacket(pkt ipPacket, chnl chan *linkMessage) {
  tid := s.TunIdForPkt(pkt)
  if tid > 0 {
    // we got a tunnel for it
    if pkt.Dst().Equal(s.addr) {
      // it's for us
      chnl <- &linkMessage{
        Addr: s.remote,
        Frame: newFrameFromPackets([]ipPacket{pkt}, tid),
      }
      // XXX: batching and clumping ?
    } else {
      log.Println("session got spoofed packet?", pkt.Src(), "->", pkt.Dst())
    }
  } else {
    log.Println("don't have tunnel for packet?", pkt.Src(), "->", pkt.Dst())
  }
}

// we got a message from i2p
func (s *linkSession) gotMessage(msg *linkMessage, replychnl chan *linkMessage) {
  switch msg.Frame.Method() {
  case ALIVE_PING:
    // TODO: record ping
    break
  case CLIENT_TUN_NEW:
    b, err := msg.Frame.GetParamStr(K_CIDR)
    if err == nil {
      _, cidr, err := net.ParseCIDR(string(b))
      if err == nil {
        if s.acceptTunnel(cidr) {
          // tunnel allowed
          t := &linkTunnel{
            tid: s.nextTunnelID(),
            allowed: cidr,
          }
          // add tunnel
          s.addTunnel(t)
          // reply
          replychnl <- &linkMessage{
            Addr: s.remote,
            Frame: newCreateClientTunnelReply(t.allowed, t.tid)
          }
        } else {
          // rejected
          replychnl <- &linkMessage{
            Addr: s.remote,
            Frame: newCreateClientTunnelReply(cidr, 0),
          }
        }
        return
      }
    }
    // error
    // TODO: reply?
    log.Println("error handling CLIENT_TUN_NEW", err)
    break
  case CLIENT_TUN_DEL:
     
  }
  
}

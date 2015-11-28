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
    cidr *net.IPNet // allowed range
}

type linkSession struct {
    remote i2p.I2PAddr // session's destination
    addr net.IP // session's IP
    tunnels map[int64]linkTunnel // all active tunnels
    sendchnl chan linkMessage
}

func (s *linkSession) TunIdForPkt(pkt ipPacket) (tid int64) {
    addr := pkt.Src()
    for _, tun := range s.tunnels {
        if tun.cidr.Contains(addr) {
            tid = tun.tid
            break
        }
    }
    return
}

// we got a packet from the internet :D
func (s *linkSession) gotInetPacket(pkt ipPacket) {
    tid := s.TunIdForPkt(pkt)
    if tid > 0 {
        // we got a tunnel for it
        if pkt.Dst().Equal(s.addr) {
            // it's for us
            s.sendchnl <- linkMessage{
                addr: s.remote,
                frame: newFrameFromPackets([]ipPacket{pkt}, tid),
            }
            // XXX: batching and clumping
        } else {
            log.Println("session got spoofed packet?", pkt.Src(), "->", pkt.Dst())
        }
    } else {
        log.Println("don't have tunnel for packet?", pkt.Src(), "->", pkt.Dst())
    }
}

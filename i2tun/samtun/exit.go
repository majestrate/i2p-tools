package samtun
//
// samtun exit server
//

import (
    i2p "github.com/majestrate/i2p-tools/sam3"
    "log"
    "net"
    "strconv"
    "time"
)

// exit server packet handler
type exitLink struct {
    mtu int
    exitIf *tunDev
    cfg *exitNetConfig
    udpcon net.PacketConn
    recvchnl chan linkMessage
    sendchnl chan linkMessage
    sessions map[string]*linkSession
}

func (link *exitLink) LinkUp() (err error) {
    cfg := link.cfg
    link.mtu = cfg.IfMTU
    log.Println("set link", cfg.IfName, "up")
    link.exitIf, err = newTun(cfg.IfName, cfg.IfAddr, cfg.IfMask, link.mtu)
    return
}

// get session for this ip packet
func (link *exitLink) sessionFor(pkt ipPacket) (s *linkSession) {
    sess, ok := link.sessions[pkt.Dst().String()]
    if ok {
        s = sess
    }
    return
}

func (link *exitLink) RunNetIf() {
    for {
        buff := make([]byte, (link.mtu + 64))
        n, err := link.exitIf.Read(buff)
        if err == nil {
            // read okay
            pkt := ipPacket(buff[:n])
            // inet packet -> client
            s := link.sessionFor(pkt)
            if s == nil {
                // no session?
                log.Println("no session for packet", pkt.Src() ,"->", pkt.Dst())
            } else {
                // session will handle inet packet
                s.gotInetPacket(pkt)
            }
        } else {
            // read error
            log.Fatal(err)
        }
    }
}

// run exit link mainloop
func (link *exitLink) Run() {
    for {
        select {
        case _ = <- link.recvchnl:
            // we got a packet from i2p
        case _ = <- link.sendchnl:
            // we got a packet to send to i2p
        }
    }
}

func (link *exitLink) Close() {
    if link.exitIf != nil {
        link.exitIf.Close()
    }
    link.udpcon.Close()
}

func createExitHandler(cfg *exitNetConfig) (link *exitLink, err error) {
    link = new(exitLink)
    link.cfg = cfg
    link.udpcon, err = net.ListenPacket("udp6", "")
    link.sendchnl = make(chan linkMessage)
    link.recvchnl = make(chan linkMessage)
    return
}

type exitServer struct {
    conf *exitNetConfig
    keys i2p.I2PKeys
    sam *i2p.SAM
}

func (serv *exitServer) Close() {
    if serv.sam != nil {
        log.Println("Closing connection to I2P")
        serv.sam.Close()
    }
}

func (serv *exitServer) Connect(cfg samConfig, udpaddr net.Addr) (s *i2p.DatagramSession, err error) {
    _, portstr, _ := net.SplitHostPort(udpaddr.String())
    port, _ := strconv.Atoi(portstr)
    if serv.sam != nil {
        log.Println("Closing existing connection to I2P")
        serv.sam.Close()
        serv.sam = nil
    }
    addr := cfg.Addr
    log.Println("Connecting to I2P via", addr)
    serv.sam, err = i2p.NewSAM(addr)
    if err == nil {
        s, err = serv.sam.NewDatagramSession(cfg.Session, serv.keys, cfg.Opts.List(), port)
    }
    return
}

func (serv *exitServer) EnsureKeys(cfg samConfig) (err error) {
    if checkfile(cfg.KeyFile) {
        log.Println("Loading Keyfile", cfg.KeyFile)
        serv.keys, err = loadI2PKey(cfg.KeyFile)
    } else {
        log.Println("Keys not found, generating new keys to", cfg.KeyFile)
        var s *i2p.SAM
        s, err = i2p.NewSAM(cfg.Addr)
        if err == nil {
            serv.keys, err = s.NewKeys()
            if err == nil {
                log.Println("Saving new keys to", cfg.KeyFile)
                err = saveI2PKey(serv.keys, cfg.KeyFile)
            }
            s.Close()
        }
    }
    return
}

func (serv *exitServer) Run() {
    for {
        time.Sleep(time.Second)
        handler, err := createExitHandler(serv.conf)
        if err == nil {
            err = handler.LinkUp()
            if err == nil {
                err = serv.EnsureKeys(serv.conf.Sam)
                // we are up
                con := handler.udpcon
                var s *i2p.DatagramSession
                s, err = serv.Connect(serv.conf.Sam, con.LocalAddr())
                if err == nil {
                    // run server
                    handler.Run()
                    s.Close()
                }
                handler.Close()
            }
        }
        if err != nil {
            log.Println(err)
        }
    }
}

func createExitNet(cfg *exitNetConfig) (serv *exitServer, err error) {
    serv = &exitServer{
        conf: cfg,
    }
    return
}

package i2p

import (
  "net"
)

//
// a session with the i2p router
//
type Session interface {
  // get base32 address string for this session
  B32() string
	// lookup the address of a name on i2p
  Lookup(name string) (net.Addr, error)
}


type StreamSession interface {
	// implements session
	Session
	// implements net.Listener
	net.Listener
  // Dial out to i2p from this session
  // same api as net.Dialer.Dial
  Dial(network, addr string) (net.Conn, error)
}

type I2PDestHash [32]byte

type PacketSession interface {
	// implements session
	Session
	// implements net.PacketConn
	net.PacketConn
}

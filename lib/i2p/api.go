package i2p

import (
  "github.com/majestrate/i2p-tools/sam3"
  "io"
  "net"
)



// session keys used for creating an i2p destination
type Keys sam3.I2PKeys

//
// a session with the i2p router
//
type Session interface {
  // implements net.Listener
  net.Listener
  // get base32 address string for this session
  B32() string
  // read private keys from an io.Reader
  ReadKeys(r io.Reader) error
  // write private keys to an io.Writer
  WriteKeys(w io.Writer) error
  // helper function for saving keys on the filesystem
  EnsureKeyfile(fname string) error
  // Dial out to i2p from this session
  // same api as net.Dialer.Dial
  Dial(network, addr string) (net.Conn, error)
  // lookup the address of a name on i2p
  Lookup(name string) (net.Addr, error)
}

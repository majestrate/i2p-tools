package i2p

import (
  "github.com/majestrate/i2p-tools/sam3"
  "io"
  "net"
)

// implements i2p.Session
type samSession struct {
  sam *sam3.SAM // used for name lookups 
  stream *sam3.StreamSession // used for connecting out
  listener *sam3.StreamListener // used for accepting inbound connections
}

// implements i2p.Session
func (s *samSession) ReadKeys(r io.Reader) (err error) {
  err = s.sam.ReadKeys(r)
  return
}

// implements i2p.Session
func (s *samSession) WriteKeys(w io.Writer) (err error) {
  k := s.sam.Keys()
  err = sam3.StoreKeysIncompat(*k, w)
  return
}

// implements i2p.Session
func (s *samSession) EnsureKeyfile(fname string) (err error) {
  _, err = s.sam.EnsureKeyfile(fname)
  return
}

// implements net.Listener
func (s *samSession) Accept() (c net.Conn, err error) {
  // lazy initialization of our listener
  if s.listener == nil {
    s.listener, err = s.stream.Listen()
  }
  if err == nil {
    c, err = s.listener.Accept()
  }
  return
}

// implements net.Listener
func (s *samSession) Addr() net.Addr {
  return s.stream.Addr()
}

// implements net.Listener
func (s *samSession) Close() (err error) {
  if s.listener != nil {
    s.listener.Close()
    s.listener = nil
  }
  if s.stream != nil {
    s.stream.Close()
    s.stream = nil
  }
  if s.sam != nil {
    s.sam.Close()
    s.sam = nil
  }
  return
}

// implements i2p.Session
func (s *samSession) B32() string {
  return s.stream.Addr().Base32()
}

// implements i2p.Session
func (s *samSession) Dial(network, addr string) (c net.Conn, err error) {
  c, err = s.stream.Dial(network, addr)
  return
}

// implements i2p.Session
func (s *samSession) Lookup(name string) (a net.Addr, err error) {
  a, err = s.sam.Lookup(name)
  return
}

// create a new session with i2p "the easy way"
func NewSessionEasy(addr, keyfile string) (session Session, err error) {
  var s *sam3.SAM
  s, err = sam3.NewSAM(addr)
  if err == nil {
    _, err = s.EnsureKeyfile(keyfile)
    if err == nil {
      var stream *sam3.StreamSession
      name := randStr(10)
      k := s.Keys()
      stream, err = s.NewStreamSession(name, *k, sam3.Options_Medium)
      if err == nil {
        session = &samSession{
          sam: s,
          stream: stream,
        }
      } else {
        s.Close()
        s = nil
      }
    } else {
      s.Close()
      s = nil
    }
  }
  return
}

// create a new session with i2p
// addr - i2p router's SAM3 interface
// name - name of the session, must be unique
// r - io.Reader that reads keys
func NewSession(addr, name string, r io.Reader) (session Session, err error) {
  var s *sam3.SAM
  s, err = sam3.NewSAM(addr)
  if err == nil {
    err = s.ReadKeys(r)
    if err == nil {
      var stream *sam3.StreamSession
      k := s.Keys()
      stream, err = s.NewStreamSession(name, *k, sam3.Options_Medium)
      if err == nil {
        session = &samSession{
          sam: s,
          stream: stream,
        }
      } else {
        s.Close()
        s = nil
      }
    } else {
      s.Close()
      s = nil
    }
  }
  return
}


func (dh I2PDestHash) String() string {
	var d sam3.I2PDestHash
	copy(d[:], dh[:])
	return d.String()
}

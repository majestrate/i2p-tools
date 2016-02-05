package sam3

import (
	"bufio"
	"bytes"
	"errors"
  "io"
	"net"
	"strings"
)

// Represents a streaming session.
type StreamSession struct {
  samAddr     string             // address to the sam bridge (ipv4:port)
	id          string             // tunnel name
  conn        net.Conn           // connection to sam
	keys        I2PKeys            // i2p destination keys
}

// Returns the local tunnel name of the I2P tunnel used for the stream session
func (ss StreamSession) ID() string {
	return ss.id
}

func (ss *StreamSession) Close() error {
  return ss.conn.Close()
}

// Returns the I2P destination (the address) of the stream session
func (ss StreamSession) Addr() I2PAddr {
	return ss.keys.Addr()
}

// Returns the keys associated with the stream session
func (ss StreamSession) Keys() I2PKeys {
	return ss.keys
}

// Creates a new StreamSession with the I2CP- and streaminglib options as 
// specified. See the I2P documentation for a full list of options.
func (sam *SAM) NewStreamSession(id string, keys I2PKeys, options []string) (*StreamSession, error) {
	conn, err := sam.newGenericSession("STREAM", id, keys, options, []string{})
	if err != nil {
		return nil, err
	}
	return &StreamSession{sam.address, id, conn, keys}, nil
}

// lookup name, convienence function
func (s *StreamSession) Lookup(name string) (I2PAddr, error) {
  sam, err := NewSAM(s.samAddr)
  if err == nil {
    addr, err := sam.Lookup(name)
    sam.Close()
    return addr, err
  }
  return I2PAddr(""), err
}

// implement net.Dialer
func (s *StreamSession) Dial(n, addr string) (c net.Conn, err error) {

  var i2paddr I2PAddr
  var host string
  host, _, err = net.SplitHostPort(addr)
  if err == nil {
    // check for name
    if strings.HasSuffix(host, ".b32.i2p") || strings.HasSuffix(host, ".i2p") {
      // name lookup
      var sam *SAM
      sam, err = NewSAM(s.samAddr)
      if err == nil {
        i2paddr, err = sam.Lookup(host)
        sam.Close()
      }
    } else {
      // probably a destination
      i2paddr = I2PAddr(host)
    }
    if err == nil {
      return s.DialI2P(i2paddr)
    }
  }
  return
}

// Dials to an I2P destination and returns a SAMConn, which implements a net.Conn.
func (s *StreamSession) DialI2P(addr I2PAddr) (*SAMConn, error) {
	sam, err := NewSAM(s.samAddr)
	if err != nil {
		return nil, err
	}
	conn := sam.conn
	_,err = conn.Write([]byte("STREAM CONNECT ID=" + s.id + " DESTINATION=" + addr.Base64() + " SILENT=false\n"))
	if err != nil {
		conn.Close()
		return nil, err
	}
	buf := make([]byte, 4096)
	n, err := conn.Read(buf)
	if err != nil {
		conn.Close()
		return nil, err
	}
	scanner := bufio.NewScanner(bytes.NewReader(buf[:n]))
	scanner.Split(bufio.ScanWords)
	for scanner.Scan() {
		switch scanner.Text() {
		case "STREAM" :
			continue
		case "STATUS" :
			continue
		case "RESULT=OK" :
			return &SAMConn{s.keys.addr, addr, conn}, nil
		case "RESULT=CANT_REACH_PEER" :
      sam.Close()
			return nil, errors.New("Can not reach peer")
		case "RESULT=I2P_ERROR" :
      sam.Close()
			return nil, errors.New("I2P internal error")
		case "RESULT=INVALID_KEY" :
      sam.Close()
			return nil, errors.New("Invalid key")
		case "RESULT=INVALID_ID" :
      sam.Close()
			return nil, errors.New("Invalid tunnel ID")
		case "RESULT=TIMEOUT" :
      sam.Close()
			return nil, errors.New("Timeout")
    default:
      sam.Close()
			return nil, errors.New("Unknown error: " + scanner.Text() + " : " + string(buf[:n]))
		}
	}
	panic("sam3 go library error in StreamSession.DialI2P()")
}

// create a new stream listener to accept inbound connections
func (s *StreamSession) Listen() (*StreamListener, error) {
  return &StreamListener{
    session: s,
    id: s.id,
    laddr: s.keys.Addr(),
  }, nil
}


type StreamListener struct {
  // parent stream session
  session *StreamSession
  // our session id
  id string
  // our local address for this sam socket
  laddr I2PAddr
}

// get our address
// implements net.Listener
func (l *StreamListener) Addr() net.Addr {
  return l.laddr
}

// implements net.Listener
func (l *StreamListener) Close() error {
  return l.session.Close()
}

// implements net.Listener
func (l *StreamListener) Accept() (net.Conn, error) {
  return l.AcceptI2P()
}

// accept a new inbound connection
func (l *StreamListener) AcceptI2P() (*SAMConn, error) {
  s, err := NewSAM(l.session.samAddr)
  if err == nil {
    // we connected to sam
    // send accept() command
    _, err = io.WriteString(s.conn, "STREAM ACCEPT ID="+l.id+" SILENT=false\n")
    // read reply
    rd := bufio.NewReader(s.conn)
    // read first line
    line, err := rd.ReadString(10)
    if err == nil {
      if strings.HasPrefix(line, "STREAM STATUS RESULT=OK") {
        // we gud read destination line
        dest, err := rd.ReadString(10)
        if err == nil {
          // return wrapped connection
          dest = strings.Trim(dest, "\n")
          return &SAMConn{
            laddr: l.laddr,
            raddr: I2PAddr(dest),
            conn: s.conn,
          }, nil
        } else {
          s.Close()
          return nil, err
        }
      } else {
        s.Close()
        return nil, errors.New("invalid sam line: "+line)
      }
    } else {
      s.Close()
      return nil, err
    }
  }
  return nil, err
}

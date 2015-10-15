package sam3

import (
	"bufio"
	"bytes"
	"errors"
	"net"
	"strconv"
	"strings"
)

// Represents a streaming session.
type StreamSession struct {
    samAddr     string             // address to the sam bridge (ipv4:port)
	id          string             // tunnel name
	conn        net.Conn           // connection to sam bridge
	keys        I2PKeys            // i2p destination keys
}

// Returns the local tunnel name of the I2P tunnel used for the stream session
func (ss StreamSession) ID() string {
	return ss.id
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

// Dials to an I2P destination and returns a SAMConn, which implements a net.Conn.
func (s *StreamSession) DialI2P(addr I2PAddr) (*SAMConn, error) {
	sam, err := NewSAM(s.samAddr)
	if err != nil {
		return nil, err
	}
	conn := sam.conn
	_,err = conn.Write([]byte("STREAM CONNECT ID=" + s.id + " DESTINATION=" + addr.Base64() + " SILENT=false\n"))
	if err != nil {
		return nil, err
	}
	buf := make([]byte, 4096)
	n, err := conn.Read(buf)
	if err != nil {
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
			return nil, errors.New("Can not reach peer")
		case "RESULT=I2P_ERROR" :
			return nil, errors.New("I2P internal error")
		case "RESULT=INVALID_KEY" :
			return nil, errors.New("Invalid key")
		case "RESULT=INVALID_ID" :
			return nil, errors.New("Invalid tunnel ID")
		case "RESULT=TIMEOUT" :
			return nil, errors.New("Timeout")
		default :
			return nil, errors.New("Unknown error: " + scanner.Text() + " : " + string(buf[:n]))
		}
	}
	panic("sam3 go library error in StreamSession.DialI2P()")
}

// Returns a listener for the I2P destination (I2PAddr) associated with the
// StreamSession.
func (s *StreamSession) Listen() (*StreamListener, error) {
	sam, err := NewSAM(s.conn.RemoteAddr().String())
	if err != nil {
		return nil, err
	}
	lhost, _, err := net.SplitHostPort(s.conn.LocalAddr().String())
	if err != nil {
		sam.Close()
		return nil, err
	}
	listener, err := net.Listen("tcp4", lhost + ":0")
	_, lport, err := net.SplitHostPort(listener.Addr().String())
	if err != nil {
		sam.Close()
		return nil, err
	}
	conn := sam.conn
	_, err = conn.Write([]byte("STREAM FORWARD ID=" + s.id + " PORT=" + lport + " SILENT=false\n"))
	if err != nil {
		conn.Close()
		return nil, err
	}
	buf := make([]byte, 512)
	n, err := conn.Read(buf)
	if err != nil {
		conn.Close()
		return nil, err
	}
	scanner := bufio.NewScanner(bytes.NewReader(buf[:n]))
	scanner.Split(bufio.ScanWords)
	for scanner.Scan() {
		switch strings.TrimSpace(scanner.Text()) {
		case "STREAM" :
			continue
		case "STATUS" :
			continue
		case "RESULT=OK" :
			port,_ := strconv.Atoi(lport)
			return &StreamListener{conn, listener, port, s.keys.Addr()}, nil
		case "RESULT=I2P_ERROR" :
			conn.Close()
			return nil, errors.New("I2P internal error")
		case "RESULT=INVALID_ID" :
			conn.Close()
			return nil, errors.New("Invalid tunnel ID")
		default :
			conn.Close()
			return nil, errors.New("Unknown error: " + scanner.Text() + " : " + string(buf[:n]))
		}
	}
	panic("sam3 go library error in StreamSession.Listener()")
}

// Implements net.Listener for I2P streaming sessions
type StreamListener struct {
	conn        net.Conn
	listener    net.Listener
	lport       int
	laddr       I2PAddr
}

const defaultListenReadLen = 516

// Accepts incomming connections to your StreamSession tunnel. Implements net.Listener
func (l *StreamListener) Accept() (*SAMConn, error) {
	conn, err := l.listener.Accept()
	if err != nil {
		return nil, err
	}
	buf := make([]byte, defaultListenReadLen)
	n, err := conn.Read(buf)
	if n < defaultListenReadLen {
		return nil, errors.New("Unknown destination type: " + string(buf[:n]))
	}
	// I2P inserts the I2P address ("destination") of the connecting peer into the datastream, followed by
	// a \n. Since the length of a destination may vary, this reads until a newline is found. At the time
	// of writing, the length is never less then, and almost always equals 516 bytes, which is why 
	// defaultListenReadLen is 516.
	if rune(buf[defaultListenReadLen - 1]) != '\n' {
		abuf := make([]byte, 1)
		for {
			n, err := conn.Read(abuf)
			if n != 1 || err != nil {
				return nil, errors.New("Failed to decode connecting peers I2P destination.")
			}
			buf = append(buf, abuf[0])
			if rune(abuf[0]) == '\n' { break }
		}
	}
	rAddr, err := NewI2PAddrFromString(string(buf[:len(buf)-1])) // the address minus the trailing newline
	if err != nil {
		conn.Close()
		return nil, errors.New("Could not determine connecting tunnels address.")
	}
	return &SAMConn{l.laddr, rAddr, conn}, nil
}

// Closes the stream session. Implements net.Listener
func (l *StreamListener) Close() error {
	err := l.listener.Close()
	err2 := l.conn.Close()
	if err2 != nil {
		return err2
	}
	return err
}

// Returns the I2P destination (address) of the stream session. Implements net.Listener
func (l *StreamListener) Addr() net.Addr {
	return l.laddr
}

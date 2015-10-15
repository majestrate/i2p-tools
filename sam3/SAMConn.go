package sam3

import (
	"time"
	"net"
)

// Implements net.Conn
type SAMConn struct {
	laddr  I2PAddr
	raddr  I2PAddr
	conn   net.Conn
}

// Implements net.Conn
func (sc SAMConn) Read(buf []byte) (int, error) {
	n, err := sc.conn.Read(buf)
	return n, err
}

// Implements net.Conn
func (sc SAMConn) Write(buf []byte) (int, error) {
	n, err := sc.conn.Write(buf)
	return n, err
}

// Implements net.Conn
func (sc SAMConn) Close() error {
	return sc.conn.Close()
}

// Implements net.Conn
func (sc SAMConn) LocalAddr() I2PAddr {
	return sc.laddr
}

// Implements net.Conn
func (sc SAMConn) RemoteAddr() I2PAddr {
	return sc.raddr
}

// Implements net.Conn
func (sc SAMConn) SetDeadline(t time.Time) error {
	return sc.conn.SetDeadline(t)
}

// Implements net.Conn
func (sc SAMConn) SetReadDeadline(t time.Time) error {
	return sc.conn.SetReadDeadline(t)
}

// Implements net.Conn
func (sc SAMConn) SetWriteDeadline(t time.Time) error {
	return sc.conn.SetWriteDeadline(t)
}



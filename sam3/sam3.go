// Library for I2Ps SAMv3 bridge (https://geti2p.com)
package sam3

import (
  "bufio"
  "bytes"
  "net"
  "errors"
  "io"
  "os"
  "strings"
)



// Used for controlling I2Ps SAMv3.
type SAM struct {
  address     string        // ipv4:port
  conn        net.Conn
  keys        *I2PKeys
}

const (
  session_OK             = "SESSION STATUS RESULT=OK DESTINATION="
  session_DUPLICATE_ID   = "SESSION STATUS RESULT=DUPLICATED_ID\n"
  session_DUPLICATE_DEST = "SESSION STATUS RESULT=DUPLICATED_DEST\n"
  session_INVALID_KEY    = "SESSION STATUS RESULT=INVALID_KEY\n"
  session_I2P_ERROR      = "SESSION STATUS RESULT=I2P_ERROR MESSAGE="
)

// Creates a new controller for the I2P routers SAM bridge.
func NewSAM(address string) (*SAM, error) {
  conn, err := net.Dial("tcp4", address)
  if err != nil {
    return nil, err
  }
  if _, err := conn.Write([]byte("HELLO VERSION MIN=3.0 MAX=3.0\n")); err != nil {
    return nil, err
  }
  buf := make([]byte, 256)
  n, err := conn.Read(buf)
  if err != nil {
    return nil, err
  }
  if string(buf[:n]) == "HELLO REPLY RESULT=OK VERSION=3.0\n" {
    return &SAM{address, conn, nil}, nil
  } else if string(buf[:n]) == "HELLO REPLY RESULT=NOVERSION\n" {
    return nil, errors.New("That SAM bridge does not support SAMv3.")
  } else {
    return nil, errors.New(string(buf[:n]))
  }
}

// if keyfile fname does not exist
func (sam *SAM) EnsureKeyfile(fname string) (keys I2PKeys, err error) {
  if fname == "" {
    // transient
    keys, err = sam.NewKeys()
    if err == nil {
      sam.keys = &keys
    }
  } else {
    // persistant
    _, err = os.Stat(fname)
    if os.IsNotExist(err) {
        // make the keys
        keys, err = sam.NewKeys()
        if err == nil {
            sam.keys = &keys
            // save keys
            var f io.WriteCloser
            f, err = os.OpenFile(fname, os.O_WRONLY | os.O_CREATE, 0600)
            if err == nil {
                err = StoreKeysIncompat(keys, f)
                f.Close()
            }
        }
    } else if err == nil {
        // we haz key file
        var f *os.File
        f, err = os.Open(fname)
        if err == nil {
            keys, err = LoadKeysIncompat(f)
            if err == nil {
                sam.keys = &keys
            }
        }
    }
  }
  return
}

// Creates the I2P-equivalent of an IP address, that is unique and only the one
// who has the private keys can send messages from. The public keys are the I2P
// desination (the address) that anyone can send messages to.
func (sam *SAM) NewKeys() (I2PKeys, error) {
  if _, err := sam.conn.Write([]byte("DEST GENERATE\n")); err != nil {
    return I2PKeys{}, err
  }
  buf := make([]byte, 8192)
  n, err := sam.conn.Read(buf)
  if err != nil {
    return I2PKeys{}, err
  }
  s := bufio.NewScanner(bytes.NewReader(buf[:n]))
  s.Split(bufio.ScanWords)

  var pub, priv string
  for s.Scan() {
    text := s.Text()
    if text == "DEST" {
      continue
    } else if text == "REPLY" {
      continue
    } else if strings.HasPrefix(text, "PUB=") {
      pub = text[4:]
    } else if strings.HasPrefix(text, "PRIV=") {
      priv = text[5:]
    } else {
      return I2PKeys{}, errors.New("Failed to parse keys.")
    }
  }
  return I2PKeys{I2PAddr(pub), priv}, nil
}

// Performs a lookup, probably this order: 1) routers known addresses, cached
// addresses, 3) by asking peers in the I2P network.
func (sam *SAM) Lookup(name string) (I2PAddr, error) {
  if _, err := sam.conn.Write([]byte("NAMING LOOKUP NAME=" + name + "\n")); err != nil {
    return I2PAddr(""), err
  }
  buf := make([]byte, 4096)
  n, err := sam.conn.Read(buf)
  if err != nil {
    return I2PAddr(""), err
  }
  if n <= 13 || !strings.HasPrefix(string(buf[:n]), "NAMING REPLY ") {
    return I2PAddr(""), errors.New("Failed to parse.")
  }
  s := bufio.NewScanner(bytes.NewReader(buf[13:n]))
  s.Split(bufio.ScanWords)

  errStr := ""
  for s.Scan() {
    text := s.Text()
    if text == "RESULT=OK" {
      continue
    } else if text == "RESULT=INVALID_KEY" {
      errStr += "Invalid key."
    } else if text == "RESULT=KEY_NOT_FOUND" {
      errStr += "Unable to resolve " + name
    } else if text == "NAME=" + name {
      continue
    } else if strings.HasPrefix(text, "VALUE=") {
      return I2PAddr(text[6:]), nil
    } else if strings.HasPrefix(text, "MESSAGE=") {
      errStr += " " + text[8:]
    } else {
      continue
    }
  }
  return I2PAddr(""), errors.New(errStr)
}

// Creates a new session with the style of either "STREAM", "DATAGRAM" or "RAW",
// for a new I2P tunnel with name id, using the cypher keys specified, with the
// I2CP/streaminglib-options as specified. Extra arguments can be specified by
// setting extra to something else than []string{}.
// This sam3 instance is now a session
func (sam *SAM) newGenericSession(style, id string, keys I2PKeys, options []string, extras []string) (net.Conn, error) {

  optStr := ""
  for _, opt := range options {
    optStr += opt + " "
  }

  conn := sam.conn
  scmsg := []byte("SESSION CREATE STYLE=" + style + " ID=" + id + " DESTINATION=" + keys.String() + " " + optStr + strings.Join(extras, " ") + "\n")
  for m, i:=0, 0; m!=len(scmsg); i++ {
    if i == 15 {
      conn.Close()
      return nil, errors.New("writing to SAM failed")
    }
    n, err := conn.Write(scmsg[m:])
    if err != nil {
      conn.Close()
      return nil, err
    }
    m += n
  }
  buf := make([]byte, 4096)
  n, err := conn.Read(buf)
  if err != nil {
    conn.Close()
    return nil, err
  }
  text := string(buf[:n])
  if strings.HasPrefix(text, session_OK) {
    if keys.String() != text[len(session_OK):len(text)-1] {
      return nil, errors.New("SAMv3 created a tunnel with keys other than the ones we asked it for")
    }
    return conn, nil //&StreamSession{id, conn, keys, nil, sync.RWMutex{}, nil}, nil
  } else if text == session_DUPLICATE_ID {
    conn.Close()
    return nil, errors.New("Duplicate tunnel name")
  } else if text == session_DUPLICATE_DEST {
    conn.Close()
    return nil, errors.New("Duplicate destination")
  } else if text == session_INVALID_KEY {
    conn.Close()
    return nil, errors.New("Invalid key")
  } else if strings.HasPrefix(text, session_I2P_ERROR) {
    conn.Close()
    return nil, errors.New("I2P error " + text[len(session_I2P_ERROR):])
  } else {
    conn.Close()
    return nil, errors.New("Unable to parse SAMv3 reply: " + text)
  }
}

// close this sam session
func (sam *SAM) Close() error {
  if err := sam.conn.Close(); err != nil {
    return err
  }
  return nil
}

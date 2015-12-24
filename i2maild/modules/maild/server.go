package maild

import (
  "github.com/majestrate/i2p-tools/i2maild/modules/config"
  "github.com/majestrate/i2p-tools/i2maild/modules/maildir"
  "github.com/majestrate/i2p-tools/i2maild/modules/smtp"
  i2p "github.com/majestrate/i2p-tools/sam3"

  "github.com/golang/glog"
  
  "bytes"
  "fmt"
  "net"
  "strings"
)

func RunMailServer() {
  var l net.Listener
  cfg := config.MailServer
  s, err := cfg.I2P.StreamSession()
  if err == nil {
    // we gud
    l, err = s.Listen()
    if err == nil {
      serv := &Server{
        stream: s,
        cfg: cfg,
      }
      // we are listening for new i2p connections
      smtp.Serve(l, serv.HandleMail, "i2maild", cfg.Hostname)
    }
  }
}

type Server struct {
  stream *i2p.StreamSession
  cfg *config.MailServerConfig
  mailbox maildir.MailDir
}

// given an email address get the i2p destination name for it
func parseFromAddr(email string) (name string) {
  idx_at := strings.Index(email, "@")
  idx_i2p := strings.LastIndex(email, ".i2p")
  name = fmt.Sprintf("smtp.%s.i2p", email[idx_at+1:idx_i2p])
  name = strings.Trim(name, ",= \t\r\n\f\b")
  return
}

func (s *Server) SenderIsValid(remote net.Addr, from string) (valid bool) {
  addr, err := i2p.NewI2PAddrFromString(remote.String())
  if err == nil {
    fromAddr := parseFromAddr(from)
    if len(fromAddr) > 0 {
      raddr, err := s.stream.Lookup(fromAddr)
      if err == nil {
        // lookup worked
        valid = raddr.String() == addr.String()
      }
    }
  }
  return
}

// called when we got mail
func (s *Server) HandleMail(remote net.Addr, from string, to []string, body []byte) {
  // check validity of sender
  if s.SenderIsValid(remote, from) {
    // load body into buffer
    buff := new(bytes.Buffer)
    buff.Write(body)
    // deliver body
    err := s.mailbox.Deliver(buff)
    if err == nil {
      // success
      glog.Infof("Delivered mail via %s from %s", remote, from)
    } else {
      glog.Errorf("failed to deliver mail via %s from %s, %s", remote, from, err)
    }
  } else {
    // drop message
    glog.Errorf("Spoofed message via %s from %s", remote, from) 
  }
}

package maild

import (
  "github.com/majestrate/i2p-tools/i2maild/modules/config"
  "github.com/majestrate/i2p-tools/i2maild/modules/maildir"
  "github.com/majestrate/i2p-tools/i2maild/modules/smtp"
  i2p "github.com/majestrate/i2p-tools/sam3"

  "github.com/golang/glog"
  
  "bytes"
  "fmt"
  "io"
  "net"
  "path/filepath"
  "strings"
  "time"
)

func RunMailServer() {
  var li2p, linet net.Listener
  cfg := config.MailServer
  glog.Info("starting up session with i2p")
  s, err := cfg.I2P.StreamSession()
  if err == nil {
    // we gud
    li2p, err = s.Listen()
    if err == nil {
      var d string
      d, err = filepath.Abs(cfg.MailDir)
      if err == nil {
        serv := &Server{
          stream: s,
          cfg: cfg,
          mailbox: maildir.MailDir(d),
        }
        serv.mailbox.Create()
        glog.Infof("starting up smtp server on i2p at %s", s.Addr().Base32())
        // we are listening for new i2p connections
        go smtp.Serve(li2p, serv.HandleI2PMail, "i2maild", cfg.Hostname)
        // start up inet smtp server handler
        linet, err = net.Listen("tcp", cfg.SmtpAddr)
        if err == nil {
          glog.Infof("starting up smtp server on inet at %s", cfg.SmtpAddr)
          go smtp.Serve(linet, serv.HandleInetMail, "i2maild", cfg.Hostname)
        } else {
          glog.Fatal("failed to listen on inet smtp", err)
        }
        
        // sleep 
        for {
          time.Sleep(time.Second)
        }
      }
    }
  }
  if err != nil {
    glog.Errorf("Did not start mail server: %s", err)
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
  if strings.HasSuffix(email, ".b32.i2p") {
    name = email[idx_at+1:]
  } else {
    idx_i2p := strings.LastIndex(email, ".i2p")    
    name = fmt.Sprintf("smtp.%s.i2p", email[idx_at+1:idx_i2p])
  }
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

// called when we get mail from inetland
// usually used for sending mail into i2p
func (s *Server) HandleInetMail(remote net.Addr, from string, to []string, body []byte) {

  glog.Infof("handle inet mail from %s", remote)
  
  if parseFromAddr(from) != s.cfg.Hostname {
    // bad from address don't send it yo
    glog.Error("Bad from address %s", from)
    return
  }
  
  // deliver to all
  for _, recip := range to {
    addr := parseFromAddr(recip)
    glog.Infof("send to %s", recip)
    if len(addr) > 0 {
      // found endpoint

      // deliver in parallel
      go func(addr string) {
        glog.Infof("dial out to %s", addr)
        conn, err := s.stream.Dial("tcp", addr+":smtp")
        if err == nil {
          // connected gud
          cl, err := smtp.NewClient(conn, addr)
          if err != nil {
            // failed to connect to smtp server
            glog.Errorf("failed to speak smtp with %s, %s", addr, err)
          } else {
            // good now send hello
            err = cl.Hello(s.cfg.Hostname)
            if err == nil {
              // defer call to quit
              defer func() {
                err := cl.Quit()
                if err != nil {
                  glog.Errorf("smtp quit error: %s", err)
                }
              }()
              // hello sent
              // now let's tell them we have mail
              err = cl.Mail(from)
              if err == nil {
                // we told them we have mail
                for _, rcpt := range to {
                  err = cl.Rcpt(rcpt)
                  if err != nil {
                    glog.Errorf("error sending recpt to %s, %s", addr, err)
                  }
                }
                if err == nil {
                  // all was gud
                  var wr io.WriteCloser
                  // now try to write body
                  wr, err = cl.Data()
                  if err == nil {
                    buff := new(bytes.Buffer)
                    buff.Write(body)
                    _, err = io.CopyBuffer(wr, buff, nil)
                    wr.Close()
                  }
                  if err == nil {
                    // all writes went okay
                    glog.Infof("mail sent to server %s", addr)
                  }
                }
              }
            }
          }
        }
        if err != nil {
          glog.Errorf("error while delivering mail: %s", err)
        }
      }(addr)
    }
  }
}

// called when we got mail from i2p
func (s *Server) HandleI2PMail(remote net.Addr, from string, to []string, body []byte) {
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
    glog.Errorf("Invalid message via %s from %s", remote, from) 
  }
}

package config

import (
  i2p "github.com/majestrate/i2p-tools/sam3"
)

// mail server configuration 
type MailServerConfig struct {
  MailDir string
  Hostname string
  I2P *i2p.Config
  SmtpAddr string
}

// default config
var MailServer = &MailServerConfig{
  SmtpAddr: "127.0.0.1:7625",
  MailDir: "./i2maild",
  Hostname: "smtp.psi.i2p",
  I2P: &i2p.Config{
    Addr: "127.0.0.1:7656",
    Session: "i2maild",
    Keyfile: "maild-privkey.dat",
  },
}

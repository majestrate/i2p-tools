//
// config.go -- samtun configuration
//
package samtun

import (
  "errors"
  "github.com/majestrate/configparser"
  "log"
)

type config map[string]string

// checks if the config has all required keys
func (self config) validate() error {
  for _, k := range []string{"ifname", "sam", "keyfile", "netmask", "srcaddr", "dstaddr", "remote", "bindether"} {
    _, ok := self[k]
    if ! ok {
      return errors.New("missing key in config: "+k)
    }
  }
  return nil
}

// generate default config
// save it to a file
func genConfig(fname string) {
  cfg := configparser.NewConfiguration()
  sect := cfg.NewSection("samtun")
  sect.Add("ifname", "i2p0")
  sect.Add("sam", "127.0.0.1:7656")
  sect.Add("keyfile", "samtun.key")
  sect.Add("netmask", "255.255.255.0")
  sect.Add("srcaddr", "10.5.0.2")
  sect.Add("dstaddr", "10.5.0.1")
  sect.Add("remote", "change-this-value")
  sect.Add("mtu", "4096")
  sect.Add("bindether", "eth0")
  sect.Add("session", "samtun")
  err := configparser.Save(cfg, fname)
  if err != nil {
    log.Fatal(err)
  }
}

// load samtun config
// does not check validity
func loadConfig(fname string) (conf config, err error) {
  cfg, err := configparser.Read(fname)
  if err == nil {
    s, err := cfg.Find("samtun")
    if err == nil {
      return s[0].Options(), nil
    }
  }
  return
}


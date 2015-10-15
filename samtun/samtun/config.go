//
// config.go -- samtun configuration
//
package samtun

import (
  "encoding/json"
  "io/ioutil"
)

type jsonConfig struct {
  Keyfile string
  Sam string
  Addr string
  Ifname string
  Session string
  Netmask string
  MTU int
  Map addrMap
}

func (conf *jsonConfig) Save(fname string) (err error) {
  var data []byte
  data, err = json.Marshal(conf)
  if err == nil {
    err = ioutil.WriteFile(fname, data, 0600)
  }
  return
}

// generate default config
func genConfig(fname string) (cfg jsonConfig) {
  cfg.Keyfile = "samtun.key"
  cfg.Sam = "127.0.0.1:7656"
  cfg.Ifname = "i2p0"
  cfg.MTU = 8192
  cfg.Addr = "10.9.0.1/24"
  cfg.Netmask = "255.255.0.0"
  cfg.Session = "samtun"
  cfg.Map = make(addrMap)
  return
}

// load samtun config
// does not check validity
func loadConfig(fname string) (conf jsonConfig, err error) {
  var data []byte
  data, err = ioutil.ReadFile(fname)
  if err == nil {
    err = json.Unmarshal(data, &conf)
  }
  return
}


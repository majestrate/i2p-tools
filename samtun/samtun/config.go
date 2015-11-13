//
// config.go -- samtun configuration
//
package samtun

import (
  "bytes"
  "encoding/json"
  "io/ioutil"
)

type samOpts map[string]string

type jsonConfig struct {
  Keyfile string
  Sam string
  SamOpts samOpts
  Addr string
  Ifname string
  Session string
  Netmask string
  MTU int
  MapFile string
}

func (conf *jsonConfig) Save(fname string) (err error) {
  var data []byte
  data, err = json.Marshal(conf)
  if err == nil {
    var buff bytes.Buffer
    err = json.Indent(&buff, data, " ", "  ")
    if err == nil {
      err = ioutil.WriteFile(fname, buff.Bytes(), 0600)
    }
  }
  return
}

// generate default config
func genConfig(fname string) (cfg jsonConfig) {
  cfg.Keyfile = "samtun.key"
  cfg.Sam = "127.0.0.1:7656"
  cfg.Ifname = "i2p0"
  cfg.MTU = 8192
  cfg.Addr = "10.10.0.1/32"
  cfg.Netmask = "255.255.0.0"
  cfg.Session = "samtun"
  cfg.MapFile = "network.json"
  cfg.SamOpts = make(samOpts)
  
  cfg.SamOpts["inbound.backupQuantity"] = "1"
  cfg.SamOpts["inbound.quantity"] = "1"
  cfg.SamOpts["inbound.length"] = "3"
  cfg.SamOpts["outbound.backupQuantity"] = "1"
  cfg.SamOpts["outbound.quantity"] = "1"
  cfg.SamOpts["outbound.length"] = "3"
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


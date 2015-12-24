package config

import (
  "bytes"
  "encoding/json"
  "io/ioutil"
)


// load config from filename
func Load(fname string) (err error) {
  var buff []byte
  buff, err = ioutil.ReadFile(fname)
  if err == nil {
    err = json.Unmarshal(buff, MailServer)
  }
  return
}

// save config to file
func Save(fname string) (err error) {
  var data []byte
  data, err = json.Marshal(MailServer)
  if err == nil {
    var buff bytes.Buffer
    err = json.Indent(&buff, data, " ", "  ")
    if err == nil {
      err = ioutil.WriteFile(fname, buff.Bytes(), 0600)
    }
  }
  return
}

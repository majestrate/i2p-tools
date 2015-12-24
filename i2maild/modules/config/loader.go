package config

import (

  "github.com/golang/glog"
  
  "bytes"
  "encoding/json"
  "io/ioutil"
  "os"
)

// return true if the file exists
func checkfile(fname string) (exists bool) {
  _, err := os.Stat(fname)
  if err == nil {
    exists = true
  } else if ! os.IsNotExist(err) {
    glog.Fatal(err)
  }
  return
}

// ensure that the config is created an loaded
func Ensure(fname string) (err error) {
  if ! checkfile(fname) {
    // does not exist generate file now
    err = Save(fname)
  }
  if err == nil {
    err = Load(fname)
  }
  return
}

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

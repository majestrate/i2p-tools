//
// i2maild main function
// copywrong you're mom 2015
//
package main

import (
  "flag"
  "github.com/majestrate/i2p-tools/i2maild/modules/config"
  "github.com/majestrate/i2p-tools/i2maild/modules/maild"
  "github.com/golang/glog"
)

var cfg_fname string

func init () {
  flag.StringVar(&cfg_fname, "config", "config.json", "configuration file")
}

func main() {

  flag.Parse()
  
  // load configuration
  err := config.Ensure(cfg_fname)
  if err == nil {
    // config load success
    maild.RunMailServer()
  } else {
    glog.Errorf("could not load config: %s", err)
  }
}

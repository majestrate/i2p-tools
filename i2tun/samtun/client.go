package samtun

import (
  "log"
)

func RunClient(args []string) {
  // defaults
  fname := "client.json"
  if len(args) > 0 {
    fname = args[0]
  }
  log.Println("starting up client")
  if ! checkfile(fname) {
    log.Println("generate default config")
    // generate a config we don't have it there
    conf := genHubConfig(fname)
    conf.Save(fname)
    log.Println("please configure the client, see", fname)
    return
  }
  log.Println("loading config file", fname)
  _ , err := loadHubConfig(fname)
  if err != nil {
    log.Fatal("failed to load config", err)
  }
  
}

package samtun

import (
  "log"
)

func RunExit(args []string) {
  // defaults
  fname := "exit.json"
  only_print_dest := false
  if len(args) > 0 {
    if args[0] == "--dest" {
      // print out destination
      only_print_dest = true
    } else {
      fname = args[0]
    }
  }
  log.Println("starting up exit")
  if ! checkfile(fname) {
    log.Println("generate default config")
    // generate a config we don't have it there
    conf := genExitConfig(fname)
    conf.Save(fname)
    log.Println("please configure the exit, see", fname)
    return
  }
  log.Println("loading config file", fname)
  cfg , err := loadExitConfig(fname)
  if err != nil {
    log.Fatal("failed to load config: ", err)
  }

  exit, err := createExitNet(&cfg.Exit)
  if err != nil {
    log.Fatal("failed to create exit ", cfg.Exit.NetName, ": ", err)
  }
  
  if only_print_dest {
    // print out destination and exit
    return
  }
  exit.Run()
  
}

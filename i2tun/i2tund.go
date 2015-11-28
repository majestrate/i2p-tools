package main

import (
  "log"
  "os"
  "github.com/majestrate/i2p-tools/i2tun/samtun"
)

func main() {
  if len(os.Args) > 1 {
    if os.Args[1] == "--exit" {
      samtun.RunExit(os.Args[2:])
    } else if os.Args[1] == "--client" {
      samtun.RunClient(os.Args[2:]) 
    } else {
      log.Printf("%s usage: [--exit|--client]\n", os.Args[0])
    }
  } else {
    log.Printf("%s usage: [--exit|--client]\n", os.Args[0])
  }
}

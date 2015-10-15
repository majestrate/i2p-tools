package samtun

import (
  "bitbucket.org/kallevedin/sam3"
  "fmt"
  "io/ioutil"
  "log"
  "os"
  "strconv"
  "strings"
  "time"
)


// determine if this file exists and we can write to it
func checkfile(fname string) bool {
  _, err := os.Stat(fname)
  return err == nil
}


func Run() {
  // defaults
  fname := "samtun.ini"

  if len(os.Args) > 1 {
    if os.Args[1] == "-h" || os.Args[1] == "--help" {
      // print usage and return
      fmt.Fprintf(os.Stdout, "usage: %s [config.ini]\n")
      return
    } else {
      fname = os.Args[1]
    }
  }
  if ! checkfile(fname) {
    // generate a config we don't have it there
    genConfig(fname)
  }
  conf, err := loadConfig(fname)
  if err == nil {
    // we loaded the config right
    err = conf.validate()
    if err == nil {
      // we are configured correctly
      samaddr := conf["sam"]
      ifname := conf["ifname"]
      keyfile := conf["keyfile"]
      remote := conf["remote"]
      mtu_str := conf["mtu"]
      session := conf["session"]
      localaddr := conf["srcaddr"]
      remoteaddr := conf["dstaddr"]
      mtu, err := strconv.ParseInt(mtu_str, 10, 32)
      iface, err := newTun(ifname, localaddr, remoteaddr, int(mtu))
      defer iface.Close()
      if err == nil {
        log.Println("connecting to", samaddr)
        sam, err := sam3.NewSAM(samaddr)
        if err == nil {
          // we have a sam connection
          var keys sam3.I2PKeys
          if ! checkfile(keyfile) {
            // we have not generated private keys yet
            log.Println("generating keys")
            keys, err = sam.NewKeys()
            log.Println("we are", keys.Addr().Base32())
            ioutil.WriteFile(keyfile, []byte(keys.String()+" "+keys.Addr().String()), 0600)
          }
          key_bytes, err := ioutil.ReadFile(keyfile)
          if err == nil {
            // read the keys
            key_str := string(key_bytes)
            privkey := strings.Split(key_str, " ")[0]
            pubkey := strings.Split(key_str, " ")[1]
            keys := sam3.NewKeys(sam3.I2PAddr(pubkey), privkey)
            // create our datagram session
            log.Println("creating session")
            dg, err := sam.NewDatagramSession(session, keys, sam3.Options_Fat, 0)
            if err == nil {
              our_addr := sam3.I2PAddr(pubkey)
              log.Println("we are", our_addr.Base32())
              // look up remote destination
              for {
                if len(remote) == 0 {
                  // not set
                  log.Println("please set the remote destination in the config")
                  return
                }
                log.Println("looking up", remote)
                remote_addr, err := sam.Lookup(remote)
                if err == nil {
                  log.Println(remote, "resolved to", remote_addr.Base32())
                  break
                } else {
                  log.Println("failed", err)
                }
                time.Sleep(time.Second)
              }
              for {
                select {
                }
              }
              log.Println("closing")
              dg.Close()
            } else {
              log.Fatal(err)
            }
          } else {
            log.Fatal(err)
          }
          
        } else {
          log.Fatal(err)
        }
      } else {
        log.Fatal(err)
      }
    } else {
      log.Fatal(err)
    }
  }
}

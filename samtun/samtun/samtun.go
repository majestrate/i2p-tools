package samtun

import (
  "bitbucket.org/majestrate/sam3"
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
            defer dg.Close()
            if err == nil {
              log.Println("session made")
              addr, err := sam.Lookup("ME")
              if err != nil {
                log.Println("failed to lookup our own destination")
                return
              }
              log.Println("we are", addr.Base32())
              // look up remote destination
              var remote_addr sam3.I2PAddr
              for {
                if len(remote) == 0 {
                  // not set
                  log.Println("please set the remote destination in the config")
                  return
                }
                log.Println("looking up", remote)
                remote_addr, err = sam.Lookup(remote)
                if err == nil {
                  log.Println(remote, "resolved to", remote_addr.Base32())
                  break
                } else {
                  log.Println("failed", err)
                }
                time.Sleep(time.Second)
              }
              log.Println("open", ifname)
              iface, err := newTun(ifname, localaddr, remoteaddr, int(mtu))
              defer iface.Close()
              tunpkt_inchnl := make(chan []byte)
              //tunpkt_outchnl := make(chan []byte)
              sampkt_inchnl := make(chan []byte)
              //sampkt_outchnl := make(chan []byte)
              // read packets from tun
              go func() {
                for {
                  pktbuff := make([]byte, mtu+8)
                  n, err := iface.Read(pktbuff[:])
                  if err == nil {
                    tunpkt_inchnl <- pktbuff[:n]
                  } else {
                    log.Println("error while reading tun packet", err)
                    close(tunpkt_inchnl)
                    return
                  }
                }
              }()

              // read packets from sam
              go func() {
                pktbuff := make([]byte, mtu+8)
                n, from, err := dg.ReadFrom(pktbuff)
                if err == nil {
                  if from.Base32() == remote_addr.Base32() {
                    // got packet from sam
                    sampkt_inchnl <- pktbuff[:n]
                  } else {
                  // unwarrented
                    log.Println("unwarrented packet from", from.Base32())
                  }
                } else {
                  log.Println("error while reading sam packet", err)
                  close(sampkt_inchnl)
                  return
                }
              }()
              done_chnl := make(chan bool)
              go func() {
                for {
                  select {
                  case pkt := <- tunpkt_inchnl:
                    _, err := dg.WriteTo(pkt, remote_addr)
                    if err == nil {
                      // we gud
                    } else {
                      log.Println("error writing to remote destination", err)
                      done_chnl <- true
                      return
                    }
                  case pkt := <- sampkt_inchnl:
                    _, err := iface.Write(pkt)
                    if err == nil {
                      // we gud
                    } else {
                      log.Println("error writing to tun interface", err)
                      done_chnl <- true
                      return
                    }
                  }
                }
              }()
              <- done_chnl
              log.Println("closing")
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

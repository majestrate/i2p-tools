package samtun

import (
  "github.com/majestrate/i2p-tools/sam3"
  "fmt"
  "io/ioutil"
  "log"
  "net"
  "os"
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
  fname := "samtun.json"
  only_print_dest := false
  if len(os.Args) > 1 {
    if os.Args[1] == "-h" || os.Args[1] == "--help" {
      // print usage and return
      fmt.Fprintf(os.Stdout, "usage: %s [config.json]\n", os.Args[0])
      return
    } else if os.Args[1] == "--dest" {
      // print out destination
      only_print_dest = true
    } else {
      fname = os.Args[1]
    }
  }
  if ! checkfile(fname) {
    // generate a config we don't have it there
    conf := genConfig(fname)
    conf.Save(fname)
  }
  conf, err := loadConfig(fname)
  if err == nil {
    // we are configured correctly
    ip, _, err := net.ParseCIDR(conf.Addr)
    if err != nil {
      log.Println("invalid address", conf.Addr)
      return 
    }
    Map := make(addrMap)
    iface, err := newTun(conf.Ifname, ip.String(), conf.Netmask, conf.MTU)
    // we have a sam connection
    var keys sam3.I2PKeys
    if ! checkfile(conf.Keyfile) {
      
      log.Println("connecting to", conf.Sam)
      sam, err := sam3.NewSAM(conf.Sam)
      if err == nil {
        // we have not generated private keys yet
        log.Println("generating keys")
        keys, err = sam.NewKeys()
        sam.Close()
        ioutil.WriteFile(conf.Keyfile, []byte(keys.String()+" "+keys.Addr().String()), 0600)
        // load addr map if it is there
        if checkfile(conf.MapFile) {
          Map, _ = loadAddrMap(conf.MapFile)
        }
        b32 := keys.Addr().Base32()
        Map[b32] = []string{conf.Addr,}
        err = saveAddrMap(conf.MapFile, Map)
        if err != nil {
          log.Println("failed to save address map", err)
          return
        }
      } else {
        log.Fatal(err)
      }
    }
    key_bytes, err := ioutil.ReadFile(conf.Keyfile)
    if err == nil {
      // read the keys
      key_str := string(key_bytes)
      privkey := strings.Split(key_str, " ")[0]
      pubkey := strings.Split(key_str, " ")[1]
      keys := sam3.NewKeys(sam3.I2PAddr(pubkey), privkey)
      log.Println("our destination is", keys.Addr().Base32())
      if only_print_dest {
        return
      }
      // load address map
      Map, err = loadAddrMap(conf.MapFile)
      if err != nil {
        log.Fatal(err)
      }
      // create our datagram session
      log.Println("creating session")
      
      var opts []string
      
      // set up sam options
      if conf.SamOpts == nil {
        opts = sam3.Options_Small
      } else {
        for k, v := range conf.SamOpts {
          opts = append(opts, fmt.Sprintf("%s=%s", k, v))
        }
      }
      sam, err := sam3.NewSAM(conf.Sam)
      if err == nil {
        dg, err := sam.NewDatagramSession(conf.Session, keys, opts, 0)
        defer dg.Close()
        if err == nil {
          log.Println("session made")
          ourAddr, err := sam.Lookup("ME")
          if err != nil {
            log.Println("failed to lookup our own destination")
            return
          }
          log.Println("we are", ourAddr.Base32())
          if err != nil {
            log.Fatalf("failed to create network interface %s", err.Error())
          }
          defer iface.Close()
          tunpkt_inchnl := make(chan []byte)
          //tunpkt_outchnl := make(chan []byte)
          sampkt_inchnl := make(chan linkMessage)
          //sampkt_outchnl := make(chan linkMessage)
          // read packets from tun
          go func() {
            for {
              pktbuff := make([]byte, conf.MTU+64)
              n, err := iface.Read(pktbuff[:])
              if err == nil {
                tunpkt_inchnl <- pktbuff[:n]
              } else {
                log.Println("error while reading tun packet", err)
                time.Sleep(time.Second)
              }
            }
          }()
          
          // read packets from sam
          go func() {
            for {
              pktbuff := make([]byte, conf.MTU+64)
              n, from, err := dg.ReadFrom(pktbuff)
              if err == nil {
                // lookup ip from address map
                if Map.AllowDest(from.Base32()) {
                  // got from sam
                  sampkt_inchnl <- linkMessage{pkt: pktbuff[:n], addr: from}
                } else {
                  // unwarrented
                  log.Println("unwarrented packet from", from.Base32())
                }
              } else {
                log.Println("error while reading sam packet", err)
                close(sampkt_inchnl)
                return
              }
            }
          }()
          lookup_chnl := make(chan string, 8)
          lookup_result_chnl := make(chan sam3.I2PAddr, 8)
          
          go func() {
            // lookup destinations goroutine
            for {
              b32 := <- lookup_chnl
              log.Println("lookup", b32)
              dest, err := sam.Lookup(b32)
              if err == nil {
                log.Println("found", b32)
                lookup_result_chnl <- dest
              }
            }
          }()
          
          done_chnl := make(chan bool)
          // sam send routine
          go func() {
            // b32 -> b64 cache
            dest_cache := make(map[string]sam3.I2PAddr)
            for {
              select {
              case addr, ok := <- lookup_result_chnl:
                if ok {
                  dest_cache[addr.Base32()] = addr
                }
                
              case data, ok := <- tunpkt_inchnl:
                if ok {
                  pkt := ipPacket(data)
                  dst := pkt.Dst()
                  b32 := Map.B32(dst)
                  if len(b32) == 0 {
                    log.Println("cannot send packet to", dst, "unknown endpoint")
                  } else {
                    dest, ok := dest_cache[b32]
                    if ok {
                      _, err := dg.WriteTo(pkt[:], dest)
                      if err == nil {
                        // we gud
                      } else {
                        log.Println("error writing to remote destination", err)
                        done_chnl <- true
                        return
                      }
                    } else {
                      // cache miss
                      // TODO: deadlock
                      lookup_chnl <- b32
                    }
                  }
                } else {
                  done_chnl <- true
                  return
                }
              }
            }
          }()

          // write tun packet routine
          go func() {
            for {
              msg, ok := <- sampkt_inchnl
              if ok {
                pkt := Map.filterMessage(msg, ourAddr)
                if pkt == nil {
                  // invalid frame D:
                  log.Println("invalid frame from", msg.addr.Base32())
                } else {
                  _, err := iface.Write(pkt)
                  if err == nil {
                    // we gud
                  } else {
                    log.Println("error writing to tun interface", err)
                    done_chnl <- true
                    return
                  }
                }
              } else {
                done_chnl <- true
                return
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
}

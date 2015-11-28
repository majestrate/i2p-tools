package samtun

import (
    i2p "github.com/majestrate/i2p-tools/sam3"
    "bytes"
    "encoding/json"
    "io"
    "io/ioutil"
    "os"
    "strings"
)

func saveI2PKey(key i2p.I2PKeys, fname string) (err error) {
    return ioutil.WriteFile(fname, []byte(key.Addr().String()+"\n"+key.String()), 0600)
}

func loadI2PKey(fname string) (key i2p.I2PKeys, err error) {
    data, err := ioutil.ReadFile(fname)
    if err == nil {
        parts := strings.Split(string(data), "\n")
        key = i2p.NewKeys(i2p.I2PAddr(parts[0]), parts[1])
    }
    return
}

// determine if this file exists and we can write to it
func checkfile(fname string) bool {
  _, err := os.Stat(fname)
  return err == nil
}


// save json in pretty fashion
func saveJson(obj interface{}, fname string) (err error) {
  var data []byte
  data, err = json.Marshal(&obj)
  if err == nil {
    var buff bytes.Buffer
    err = json.Indent(&buff, data, " ", "  ")
    if err == nil {
      err = ioutil.WriteFile(fname, buff.Bytes(), 0600)
    }
  }
  return
}


// ensure that all of d is written to w
func writeAll(d []byte, w io.Writer) (err error) {
    i := 0
    n := 0
    l := len(d)
    for i < l && err != nil {
        n, err = w.Write(d[i:])
        i += n
    }
    return
}

// read exactly n bytes from a reader
func readExactly(n int , r io.Reader) (b []byte, err error) {
    b = make([]byte, n)
    _, err = io.ReadFull(r, b)
    return 
}

package maildir

import (

  "github.com/golang/glog"
  
  "fmt"
  "io"
  "os"
  "path/filepath"
  "time"
)

// maildir mailbox protocol

type MailDir string

func (d MailDir) String() (str string) {
  str = string(d)
  return
}

func (d MailDir) Create() (err error) {
  err = os.Mkdir(d.String(), 0700)
  err = os.Mkdir(filepath.Join(d.String(), "new"), 0700)
  err = os.Mkdir(filepath.Join(d.String(), "cur"), 0700)
  err = os.Mkdir(filepath.Join(d.String(), "tmp"), 0700)
  return
}

// get a string of the current filename to use
func (d MailDir) File() (fname string) {
  hostname, err := os.Hostname()
  if err == nil {
    fname = fmt.Sprintf("%d.%d.%s", time.Now().Unix(), os.Getpid(), hostname)
  } else {
    glog.Fatal("hostname() call failed", err)
  }
  return
}

func (d MailDir) TempFile() (fname string) {
  fname = d.Temp(d.File())
  return
}

func (d MailDir) Temp(fname string) (f string) {
  f = filepath.Join(d.String(), "tmp", fname)
  return
}

func (d MailDir) NewFile() (fname string) {
  fname = d.New(d.File())
  return
}

func (d MailDir) New(fname string) (f string) {
  f = filepath.Join(d.String(), "new", fname)
  return
}

// deliver mail to this maildir
func (d MailDir) Deliver(body io.Reader) (err error) {
  var oldwd string
  oldwd, err = os.Getwd()
  if err == nil {
    // no error getting working directory, let's begin

    // when done chdir to previous directory
    defer func(){
      err := os.Chdir(oldwd)
      if err != nil {
        glog.Fatal("chdir failed", err)
      }
    }()
    // chdir to maildir
    err = os.Chdir(d.String())
    if err == nil {
      fname := d.File()
      for {
        _, err = os.Stat(d.Temp(fname))
        if os.IsNotExist(err) {
          break
        }
        time.Sleep(time.Second * 2)
        fname = d.TempFile()
      }
      // set err to nil
      err = nil
      var f *os.File
      // create tmp file
      f, err = os.Create(d.Temp(fname))
      if err == nil {
        // success creation
        err = f.Close()
      }
      // try writing file
      if err == nil {
        f, err = os.OpenFile(d.Temp(fname), os.O_CREATE | os.O_WRONLY, 0600)
        if err == nil {
          // write body
          _, err = io.CopyBuffer(f, body, nil)
          f.Close()
          if err == nil {
            // now symlink
            err = os.Symlink(filepath.Join("tmp", fname), filepath.Join("new", fname))
            // if err is nil it's delivered
          }
        }
      }
    }
  }
  return
}


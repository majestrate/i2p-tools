package samtun

/*
  #include <stdlib.h>
  #include <unistd.h>
 
  ssize_t uchar_read(int fd, unsigned char * d, size_t l)
  {
    return read(fd, d, l);
  }

  ssize_t uchar_write(int fd, unsigned char * d, size_t l)
  {
    return write(fd, d, l);
  }

*/
import "C"

import "errors"

// read from a file descriptor
func fdRead(fd C.int, d []byte) (n int, err error) {
  buff := make([]C.uchar, len(d))
  res := C.uchar_read(fd, &buff[0], C.size_t(len(d)))
  if res == C.ssize_t(-1) {
    err = errors.New("read returned -1")
  } else {
    n = int(res)
    for i, b := range buff[:n] {
      d[i] = byte(b)
    }
  }
  return
}

// write to a file descriptor
func fdWrite(fd C.int, d []byte) (n int, err error) {
  buff := make([]C.uchar, len(d))
  for i, b := range d {
    buff[i] = C.uchar(b)
  }
  res := C.uchar_write(fd, &buff[0], C.size_t(len(d)))
  if res == C.ssize_t(-1) {
    err = errors.New("write returned -1")
  } else {
    n = int(res)
  }
  return
}

// close a file descriptor
func fdClose(fd C.int) (err error) {
  res := C.close(fd)
  if res == C.int(-1) {
    err = errors.New("close returned -1")
  }
  return
}

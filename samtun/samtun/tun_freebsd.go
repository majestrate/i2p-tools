// +build freebsd
// tun_freebsd.go -- tun interface with cgo for linux / bsd
//

package samtun

/*

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_tun.h>
#include <stdio.h>

int tundev_open(char * ifname) {
  if (strlen(ifname) > IFNAMSIZ) {
    return -1;
  }
  char name[IFNAMSIZ];
  sprintf(name, "/dev/%s", ifname);
  int fd = open(name, O_RDWR);
  if (fd > 0) {
    int i = 0;
    ioctl(fd, TUNSIFHEAD, &i);
  }
  return fd;
}

int tundev_up(char * ifname, char * addr, char * netmask, int mtu) {

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if ( fd > 0 ) {
    ifr.ifr_mtu = mtu;
    if ( ioctl(fd, SIOCSIFMTU, (void*) &ifr) < 0) {
      close(fd);
      perror("SIOCSIFMTU");
      return -1;
    }

    struct sockaddr_in src;
    memset(&src, 0, sizeof(struct sockaddr_in));
    src.sin_family = AF_INET;
    if ( ! inet_aton(addr, &src.sin_addr) ) {
      printf("invalid srcaddr %s\n", addr);
      close(fd);
      return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memcpy(&ifr.ifr_addr, &src, sizeof(struct sockaddr_in));
    if ( ioctl(fd, SIOCSIFADDR, (void*)&ifr) < 0 ) {
      close(fd);
      perror("SIOCSIFADDR");
     return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if ( ioctl(fd, SIOCGIFFLAGS, (void*)&ifr) < 0 ) {
      close(fd);
      perror("SIOCGIFFLAGS");
      return -1;
    }
    ifr.ifr_flags |= IFF_UP ;
    if ( ioctl(fd, SIOCSIFFLAGS, (void*)&ifr) < 0 ) {
      perror("SIOCSIFFLAGS");
      close(fd);
      return -1;
    }

    close(fd);
    return 0;
  } 
  return -1;
}

void tundev_close(int fd) {
  close(fd);
}

*/
import "C"

import (
  "errors"
)

type tunDev struct {
  fd C.int
}

func newTun(ifname, addr, dstaddr string, mtu int) (t tunDev, err error) {
  fd := C.tundev_open(C.CString(ifname))
  
  if fd == -1 {
    err = errors.New("cannot open tun interface")
  } else {
    if C.tundev_up(C.CString(ifname), C.CString(addr), C.CString(dstaddr), C.int(mtu)) < C.int(0) {
      err = errors.New("cannot put up interface")
    } else {
      t = tunDev{fd}
    }
  }
  return
}

// read from the tun device
func (t *tunDev) Read(d []byte) (n int, err error) {
  return fdRead(C.int(t.fd), d)
}

func (t *tunDev) Write(d []byte) (n int, err error) {
  return fdWrite(C.int(t.fd), d)
}


func (t *tunDev) Close() {
  C.tundev_close(C.int(t.fd))
}

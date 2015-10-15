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

int tundev_open(char * ifname) {
  int fd = open("/dev/net/tun", O_RDWR);
  if (fd > 0) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_POINTOPOINT;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if ( ioctl(fd , TUNSETIFF, (void*) &ifr) < 0) {
      close(fd);
      return -1;
    }
  }
  return fd;
}

int tundev_up(char * ifname, char * addr, char * dstaddr, int mtu) {

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  int fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
  if ( fd > 0 ) {
    if ( ioctl(fd, SIOCGIFINDEX, (void*) &ifr) < 0 ) {
      perror("SIOCGIFINDEX");
      close(fd);
      return -1;
    }
    ifr.ifr_mtu = mtu;
    if ( ioctl(fd, SIOCSIFMTU, (void*) &ifr) < 0) {
      close(fd);
      perror("SIOCSIFMTU");
      return -1;
    }
    if ( ioctl(fd, SIOCGIFFLAGS, (void*)&ifr) < 0 ) {
      close(fd);
      perror("SIOCGIFFLAGS");
      return -1;
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if ( ioctl(fd, SIOCSIFFLAGS, (void*)&ifr) < 0 ) {
      close(fd);
      return -1;
    }
    //struct sockaddr_in dst;
    //memset(&dst, 0, sizeof(dst));
    //inet_aton(dstaddr, &dst.sin_addr);
    //struct sockaddr_in src;
    //memset(&src, 0, sizeof(src));
    //inet_aton(addr, &src.sin_addr);
    //memcpy(&ifr.ifr_addr, &src, sizeof(src));
    //memcpy(&ifr.ifr_dstaddr, &dst, sizeof(dst));
    //if ( ioctl(fd, SIOCSIFADDR, (void*)&ifr) < 0 ) {
    //  close(fd);
    //  perror("SIOCSIFADDR");
    // return -1;
    //}

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

type tunDev C.int

func newTun(ifname, addr, dstaddr string, mtu int) (t tunDev, err error) {
  fd := C.tundev_open(C.CString(ifname))
  
  if fd == -1 {
    err = errors.New("cannot open tun interface")
  } else {
    if C.tundev_up(C.CString(ifname), C.CString(addr), C.CString(dstaddr), C.int(mtu)) < C.int(0) {
      err = errors.New("cannot put up interface")
    } else {
      return tunDev(fd), nil
    }
  }
  return -1, err
}


func (d tunDev) Close() {
  C.tundev_close(C.int(d))
}

//
// tun.go -- tun interface with cgo for linux / bsd
//

package samtun

/*

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int tundev_open(char * ifname) {
  int fd = open("/dev/net/tun", O_RDWR);
  if (fd > 0) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
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
    if ( ioctl(fd, SIOCGIFINDEX, &ifr) < 0 ) {
      close(fd);
      return -1;
    }
    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    inet_aton(dstaddr, &dst.sin_addr);
    struct sockaddr_in src;
    memset(&src, 0, sizeof(src));
    inet_aton(addr, &src.sin_addr);
    memcpy(&ifr.ifr_addr, &src, sizeof(src));
    memcpy(&ifr.ifr_dstaddr, &dst, sizeof(dst));
    if ( ioctl(fd, SIOCSIFADDR, &ifr) < 0 ) {
      close(fd);
      return -1;
    }
    ifr.ifr_mtu = mtu;
    if ( ioctl(fd, SIOCSIFMTU, &ifr) < 0) {
      close(fd);
      return -1;
    }
    if ( ioctl(fd, SIOCGIFFLAGS, &ifr) < 0 ) {
      close(fd);
      return -1;
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if ( ioctl(fd, SIOCSIFFLAGS, &ifr) < 0 ) {
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


type tunDev C.int

func newTun(ifname, addr, dstaddr string) (t tunDev, err error) {
  t = C.tundev_open(C.CString(ifname))
  if t == -1 {
    err = errors.New("cannot open tun interface")
  } else {
    if C.tundev_up(C.CString(ifname), C.CString(addr), C.CString(dstaddr), C.int(mtu)) < 0 {
      err = errors.New("cannot put up interface")
    }
  }
  return 
}


func (d tunDev) Close() {
  C.tundev_close(d)
}

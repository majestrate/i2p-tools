package samtun
//
// samtun ethernet driver for linux
//

/*

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define SAMTUN_ETHER_PROTO 0xDEAD

// open ethernet given interface name
int ether_open() {
  return socket(AF_PACKET, SOCK_RAW, htons(SAMTUN_ETHER_PROTO));
}

size_t ether_recv(int fd, unsigned char * result) {
  //struct sockaddr_ll addr;
  //memset(&addr, 0, sizeof(struct sockaddr_ll));
  //addr.sll_hatype = ARPHRD_ETHER;
  //addr.sll_pkttype = PACKET_BROADCAST;
  return recvfrom(fd, result, ETH_FRAME_LEN, 0, NULL, NULL);
}

// send regular ethernet frame 
int ether_send(int fd, int if_idx, const unsigned char * if_srcaddr, const unsigned char * if_dstaddr, const unsigned char * dataptr, const size_t datalen) {
  if ( datalen + 14 > ETH_FRAME_LEN ) {
    // invalid size
    return -1;
  }
  char frame[ETH_FRAME_LEN];

  char* head = frame;
  char* data = head + 14;
  struct ethhdr * eh = (struct ethhdr *) head;
  memcpy(data, dataptr, datalen);

  // sendto address
  struct sockaddr_ll addr;
  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = if_idx;
  addr.sll_halen = ETH_ALEN;
  memcpy(addr.sll_addr, if_dstaddr, ETH_ALEN);

  // destination macaddr
  memcpy(eh->h_dest, if_dstaddr, ETH_ALEN);
  // source addr is our network interface
  memcpy(eh->h_source, if_srcaddr, ETH_ALEN);
  // ethernet protocol 
  eh->h_proto = htons(SAMTUN_ETHER_PROTO);

  return sendto(fd, (void*)&frame, datalen + 14, 0, (struct sockaddr *)&addr, sizeof(addr));
}

// send ethernet broadcast frame
int ether_broadcast(int fd, int if_idx, const unsigned char * if_hwaddr, const unsigned char * dataptr, const size_t datalen) {

  if ( datalen + 14 > ETH_FRAME_LEN ) {
    // invalid size
    return -1;
  }
  char frame[ETH_FRAME_LEN];

  char* head = frame;
  char* data = head + 14;
  struct ethhdr * eh = (struct ethhdr *) head;
  memcpy(data, dataptr, datalen);

  // broadcast address
  struct sockaddr_ll addr;
  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = if_idx;
  addr.sll_halen = ETH_ALEN;
  memset(addr.sll_addr, 0xff, ETH_ALEN);

  // broadcast dest addr
  memset(eh->h_dest, 0xff, ETH_ALEN);
  // source addr is our network interface
  memcpy(eh->h_source, if_hwaddr, ETH_ALEN);
  // ethernet protocol is 0xdead
  eh->h_proto = htons(SAMTUN_ETHER_PROTO);

  return sendto(fd, (void*)&frame, datalen + 14, 0, (struct sockaddr *)&addr, sizeof(addr));
}

void ether_close(int fd) {
  if (fd != -1) close(fd);
}

*/
import "C"

import (
    "errors"
    "net"
)

type etherDev struct {
    fd C.int
    iface *net.Interface
    hwaddr [6]C.uchar
}

func hwAddrToBytes(addr net.HardwareAddr) (b [6]C.uchar) {
    for n, c := range addr {
        b[n] = C.uchar(c)
    }
    return
}

func bytesToUchar(data []byte) (d []C.uchar) {
    d = make([]C.uchar, len(data))
    for i, c := range data {
        d[i] = C.uchar(c)
    }
    return 
}

// send ethernet broadcast data from us
func (dev *etherDev) Broadcast(data []byte) (err error) {
    d := bytesToUchar(data)
    res := C.ether_broadcast(dev.fd, C.int(dev.iface.Index), &dev.hwaddr[0], &d[0], C.size_t(len(data)))
    if res == -1 {
        err = errors.New("failed to broadcast ethernet frame")
    }
    return
}

func (dev * etherDev) SendTo(dst net.HardwareAddr, data []byte) (err error) {
    addr := hwAddrToBytes(dst)
    d := bytesToUchar(data)
    res := C.ether_send(dev.fd, C.int(dev.iface.Index), &dev.hwaddr[0], &addr[0], &d[0], C.size_t(len(data)))
    if res == -1 {
        err = errors.New("failed to send ethernet frame");
    }
    return
}

func newEth(iface string) (dev *etherDev, err error) {
    d := &etherDev{
        fd: -1,
        iface: nil,
    }
    // obtain network interface
    d.iface, err = net.InterfaceByName(iface)
    if err == nil {
        if len(d.iface.HardwareAddr) == 6 {
            // we have an ethernet based interface yey
            // now we get out hardware address 
            // XXX: what happens if we change our mac suddenly?
            d.hwaddr = hwAddrToBytes(d.iface.HardwareAddr)
            // open file descriptor
            d.fd = C.ether_open()
            if d.fd == -1 {
                // failed
                err = errors.New("cannot bind to "+iface)
            } else {
                // success
                dev = d
            }
        } else {
            // failed
            err = errors.New("hardware address length != 6 bytes")
        }
    }
    return
}

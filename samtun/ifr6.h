#ifndef IFR6_H_
#define IFR6_H_
#include <netinet/in.h>

// for ipv6 ifreq
struct in6_ifreq {
  struct in6_addr ifr6_addr;
  __u32 ifr6_prefixlen;
  unsigned int ifr6_ifindex;
};


#endif

//
// address.hpp
// copywrong you're mom 2015
//
#ifndef ADDRESS_HPP
#define ADDRESS_HPP
#include <string>
#include <cstring>
#include <netinet/in.h>


struct i2p_b32addr_t {
  uint8_t m_data[32];

  // empty address
  i2p_b32addr_t();
  
  // computes the sha256 of this destination blob
  i2p_b32addr_t(std::string destblob);

  // compute the ipv6 address for this destination hash
  // uses 0200::/8 address block (supposivly deprecated)
  // The next 15 bytes are bytes 1 to 16 of our has value
  operator in6_addr();

  operator uint8_t * ();
  
  // .b32.i2p string representation
  operator std::string();

  bool operator ==(i2p_b32addr_t & other);
  
};

// convert an ipv6 address to string
std::string addr_tostring(in6_addr addr);

#endif

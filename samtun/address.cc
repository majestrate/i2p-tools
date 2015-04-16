//
// address.cc
// copywrong you're mom 2015
//
#include "address.hpp"
#include "base64.hpp"
#include "sha2.h"
#include <cstring>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sha2.h"
#include "base64.hpp"

std::string addr_tostring(in6_addr addr) {
  char addrbuff[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &addr, addrbuff, INET6_ADDRSTRLEN);
  return std::string(addrbuff);
}

i2p_b32addr_t::operator std::string() {
  return base32_encode(m_data, sizeof(m_data)) + ".b32.i2p";
}

i2p_b32addr_t::operator in6_addr() {
  in6_addr addr;
  memcpy(&addr.s6_addr, m_data, 16);
  addr.s6_addr[0] = 0x02;
  return addr;
}

i2p_b32addr_t::operator uint8_t * () {
  return m_data;
}

bool i2p_b32addr_t::operator==(i2p_b32addr_t &other) {
  return memcmp(other.m_data, m_data, sizeof(m_data));
}

i2p_b32addr_t::i2p_b32addr_t() { memset(m_data, 0, sizeof(m_data)); }


i2p_b32addr_t::i2p_b32addr_t(std::string destblob) {
    size_t unpack_len;
    uint8_t * unpack = base64_decode(destblob, &unpack_len);
    sha256(unpack, unpack_len, m_data);
    delete unpack;
}

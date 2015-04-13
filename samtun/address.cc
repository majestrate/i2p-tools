//
// samtun ipv4 address deriviation
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

void compute_address(std::string destination, in6_addr * addr)
{
  uint8_t * digest = new uint8_t[32];
  size_t destination_unpack_len;
  uint8_t * destination_unpack = base64_decode(destination, &destination_unpack_len);
  sha256(destination_unpack, destination_unpack_len, digest);
  memcpy(&addr->s6_addr, digest, 16);
  addr->s6_addr[0] = 0x02;
  delete digest;
  delete destination_unpack;
}


std::unordered_map<std::string, std::string> cache;
std::mutex cache_mutex;

std::string addr_tostring(in6_addr addr) {
  char addrbuff[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &addr, addrbuff, INET6_ADDRSTRLEN);
  return std::string(addrbuff);
}

bool cached_destination(std::string dest) {

  in6_addr addr;
  compute_address(dest, &addr);
  // obtain lock
  std::lock_guard<std::mutex> lock(cache_mutex);
  return cache.find(addr_tostring(addr)) != cache.end();
}

void save_endpoint(std::string dest) {
  in6_addr addr;
  compute_address(dest, &addr);
  auto key = addr_tostring(addr);
  // obtain lock
  std::lock_guard<std::mutex> lock(cache_mutex);
  cache[key] = dest;
}

std::string lookup_destination(in6_addr addr) {
  auto key = addr_tostring(addr);
  
  std::lock_guard<std::mutex> lock(cache_mutex);
  if (cache.find(key) == cache.end()) {
    return "";
  }
  return cache[key];
}

void persist_cache() {
  std::cout << "persiting cache" << std::endl;
  // obtain lock
  std::lock_guard<std::mutex> lock(cache_mutex);
  std::ofstream f;
  f.open("nodes.txt");
  if (!f.is_open()) {
    std::cerr << "cannot open nodes.txt";
    std::cerr << std::endl;
  }
  for (auto key : cache) {
    f << key.second << std::endl;
  }
  f.flush();
  f.close();
}

void restore_cache() {
  std::cout << "restore cache" << std::endl;
  std::ifstream f;
  f.open("nodes.txt");
  if (!f.is_open()) {
    // can't open cache
    // whatever, just return
    return;
  }
  
  std::string dest;
  do {
    dest.clear();
    std::getline(f, dest);
    // strip out whitespaces
    auto itr = dest.find(' ');
    while(itr != std::string::npos ) {
      dest.erase(itr);
      itr = dest.find(' ');
    }
    save_endpoint(dest);
  } while(!dest.empty());
}

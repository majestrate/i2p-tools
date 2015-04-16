//
// dht.hpp
// copywrong you're mom 2015
//
#ifndef DHT_HPP
#define DHT_HPP
#include <vector>
#include <mutex>
#include <cstdint>
#include "address.hpp"

const char dht_byte = 0x5f;

constexpr size_t dht_kbucket_count = 16;

// write to someone remotely
typedef int (*WriteFunction)(std::string, void*, size_t);

typedef std::string DHT_Val_t; // i2p destblob
typedef std::array<uint8_t, 16> DHT_Key_t; // ipv6 address
typedef std::array<uint8_t, 16> DHT_KeyDistance_t; // distance between keys

typedef std::vector<DHT_Val_t> DHT_KBucket_t;

void KBucketPut(DHT_KBucket_t & bucket, DHT_Val_t & val);
bool KBucketNonEmpty(DHT_KBucket_t & bucket);
DHT_Val_t KBucketGetClosest(DHT_KBucket_t & bucket, DHT_Key_t & key);


typedef std::array<DHT_KBucket_t, dht_kbucket_count> DHT_RoutingTable_t;

typedef std::pair<DHT_Key_t, DHT_Val_t> DHT_Request_t;

typedef std::vector<DHT_Request_t> DHT_PendingTable_t;

typedef DHT_PendingTable_t::iterator DHT_Pending_t;


// return the distance between two keys
DHT_KeyDistance_t ComputeKademlia(DHT_Key_t & k1, DHT_Key_t & k2);


// count the number of set bits in a distance
uint16_t CountDistanceBits(DHT_KeyDistance_t & dist);

// dht handler 
struct DHT_t {

  DHT_RoutingTable_t routing_table;
  DHT_PendingTable_t remote_pending;
  DHT_PendingTable_t local_pending;

  in6_addr node_addr;
  
  DHT_t();
  ~DHT_t();


  // initialize with our node's destination
  void Init(std::string & our_dest);
  
  // handle raw dht request
  void HandleData(std::string & fromaddr, char * buff, size_t bufflen, WriteFunction write);

  // restore from file
  void Restore(std::string fname);
  // persist to file
  void Persist(std::string fname);

  // do we know the info for this peer?
  bool KnownDest(i2p_b32addr_t & addr);

  // do we know the destination for this address?
  bool KnownAddr(in6_addr & addr);

  // return the destination for a locally known address
  std::string GetDest(in6_addr & addr);
  
  // put a destination blob into our routing table
  // we now know it locally
  void Put(std::string & destblob);

  // send out a request
  // hopefully eventually the node with this ipv6 address will reply to us
  void Find(in6_addr & addr, WriteFunction write);

  //
  // begin internal members
  //
  
  // obtain a kbucket closest to this address with stuff in it
  DHT_KBucket_t & getKBucketForAddr(in6_addr & addr);
  // obtain the closest known peer for this ipv6 address 
  DHT_Request_t getClosestForAddr(in6_addr & addr);

  DHT_Pending_t getLocalPending(DHT_Key_t & key);
  DHT_Pending_t getRemotePending(DHT_Key_t & key);
  
};

#endif

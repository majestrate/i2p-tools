//
// dht.hpp
// copywrong you're mom 2015
//
#ifndef DHT_HPP
#define DHT_HPP
#include <vector>
#include <mutex>
#include <cstdint>
#include <unordered_map>
#include "address.hpp"

const char dht_byte = 0x00;

constexpr size_t dht_kbucket_count = 16;

// write to someone remotely
typedef int (*WriteFunction)(std::string, void*, size_t);

typedef std::string DHT_Val_t; // i2p destblob

// ipv6 address
class DHT_Key_t {
private:
  in6_addr addr;
public:
  uint8_t * data() const {
    return (uint8_t*)addr.s6_addr;
  }

  DHT_Key_t() { memset(addr.s6_addr, 0, sizeof(addr.s6_addr)); }
  
  operator std::string () {
    return addr_tostring(addr);
  }

  operator const std::string () const {
    return addr_tostring(addr);
  }
  
  size_t size() const {
    return sizeof(addr.s6_addr);
  }

  uint8_t operator[](size_t idx) {
    return addr.s6_addr[idx];
  }

  bool operator==(const DHT_Key_t & other) const {
    return memcmp(addr.s6_addr, other.addr.s6_addr, sizeof(addr.s6_addr)) == 0;
  }
  operator in6_addr () {
    return addr;
  }  
};

template <class T> class Hash_DHT_Key_t;

template <> class Hash_DHT_Key_t<DHT_Key_t> {
public:
  std::size_t operator()(const DHT_Key_t & key) const {
    std::size_t h = std::hash<std::string>()(key);
    return h;
  }
};

template <class T> class Equal_DHT_Key_t;
template <> class Equal_DHT_Key_t<DHT_Key_t> {
public:
  bool operator()(const DHT_Key_t & k1, const DHT_Key_t &k2) const {
    return memcmp(k1.data(), k2.data(), k1.size()) == 0;
  }
};

// distance between keys
typedef std::array<uint8_t, 16> DHT_KeyDistance_t; 

// ( target, reply_to)
typedef std::pair<DHT_Key_t, DHT_Val_t> DHT_Request_t;


// maps ipv6 address to destination blob
typedef std::vector<DHT_Request_t> DHT_KBucket_t;

void KBucketPut(DHT_KBucket_t * bucket, DHT_Val_t & val);
bool KBucketNonEmpty(DHT_KBucket_t * bucket);
bool KBucketContainsKey(DHT_KBucket_t * bucket, DHT_Key_t & key);
bool KBucketContainsVal(DHT_KBucket_t * bucket, DHT_Val_t & val);
DHT_Request_t KBucketGetClosest(DHT_KBucket_t * bucket, DHT_Key_t &k);


// our routing table
typedef std::array<DHT_KBucket_t, dht_kbucket_count> DHT_RoutingTable_t;

// ( requester , dht request )
typedef std::pair<DHT_Val_t, DHT_Request_t> DHT_PendingRequest_t;

// table of pending operations
typedef std::vector<DHT_PendingRequest_t> DHT_PendingRequestTable_t;

typedef DHT_PendingRequestTable_t::iterator DHT_Operation_t;

// nodes we have tried
typedef std::vector<DHT_Key_t> DHT_RequestHistory_t;
// map of request target to nodes we have tried
typedef std::unordered_map<DHT_Key_t, DHT_RequestHistory_t, Hash_DHT_Key_t<DHT_Key_t>, Equal_DHT_Key_t<DHT_Key_t> > DHT_RequestTable_t;

// record request history
void RequestHistoryPut(DHT_RequestTable_t * req_table, DHT_Key_t & target, DHT_Key_t & tried);

// forget all request history for target
void RequestHistoryForget(DHT_RequestTable_t * req_table, DHT_Key_t & target);

// return true if the request history contains a key
bool RequestHistoryContains(DHT_RequestHistory_t * history, DHT_Key_t & target);

// obtain the request history for a target, creates if it doesn't exist
DHT_RequestHistory_t * RequestHistoryGet(DHT_RequestTable_t * req_table, DHT_Key_t & target);

bool KBucketGetClosestExclude(DHT_KBucket_t * bucket, DHT_RequestHistory_t * history, DHT_Key_t &k, DHT_Request_t * result);


// return the distance between two keys
DHT_KeyDistance_t ComputeKademlia(DHT_Key_t & k1, DHT_Key_t & k2);


// count the number of set bits in a distance
uint16_t CountDistanceBits(DHT_KeyDistance_t & dist);

// dht handler 
struct DHT_t {

  // our routing table
  DHT_RoutingTable_t routing_table;

  // remote pending requests
  DHT_PendingRequestTable_t remote_pending;
  // local pending requests
  DHT_PendingRequestTable_t local_pending;
  // table for keeping tack of who we tried
  DHT_RequestTable_t request_history;
  
  in6_addr node_addr;
  DHT_Val_t node_dest;

  
  bool verbose;
  
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
  bool KnownDest(std::string & destblob);

  // do we know the destination for this address?
  bool KnownAddr(in6_addr & addr);

  // return the destination for a locally known address
  std::string GetDest(in6_addr & addr);
  std::string GetDest(DHT_Key_t & k);
  
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
  DHT_KBucket_t * getKBucketForAddr(in6_addr & addr);
  // obtain a kbucket for this destination
  DHT_KBucket_t * getKBucketForDest(DHT_Val_t & dest);
  // obtain the closest known peer for this ipv6 address 
  DHT_Request_t getClosestForAddr(in6_addr & addr);
  // get next closest key for our target
  // return true if we have one and set the result
  // otherwise return false
  bool getNextClosest(DHT_Key_t & target, DHT_Key_t * result);

  // get tried nodes for a given target address
  DHT_RequestHistory_t * getRequestHistoryForAddr(DHT_Key_t & k);
  
};

#endif

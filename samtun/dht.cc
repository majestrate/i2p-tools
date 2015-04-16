#include "dht.hpp"
#include "address.hpp"
#include <iostream>
#include <fstream>

#define DHT_FILENAME "nodes.txt"

DHT_t::DHT_t() {
  Restore(DHT_FILENAME);
}

DHT_t::~DHT_t() {
  Persist(DHT_FILENAME);
}

void DHT_t::Init(std::string & our_dest) {
  // set our node address
  node_addr = i2p_b32addr_t(our_dest);
  // put our destination in bucket 0
  KBucketPut(routing_table[0], our_dest);
}

void DHT_t::Restore(std::string fname) {
  std::cout << "restore cache" << std::endl;
  std::ifstream f;
  f.open(fname);
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
    Put(dest);
  } while(!dest.empty());
}


void DHT_t::Persist(std::string fname) {
  std::ofstream f;
  f.open(fname);
  if (!f.is_open()) {
    std::cerr << "cannot open " << fname;
    std::cerr << std::endl;
  }
  
  // for all nodes write their destination blobs with a newline
  for ( auto & kbucket : routing_table ) {
    for (auto & key : kbucket) {
      if (key.second.size() > 0) {
        f << key.second << std::endl;
      }
    }

  }
  f.flush();
  f.close();
}

// dht request types

// if this packet's target matches us
// reply to thier destination with a
// dht_repl_found packet otherwise relay it
const char dht_req_get = 'G';
// get someone's capabilities
const char dht_req_caps = '?';

// dht reply types

// the request was rejected
const char dht_repl_reject = 'X';
// the request was accepted
const char dht_repl_accept = 'A';
// we are the target that was found
const char dht_repl_found = 'F';
// we have capabilities XXX: we don't use this
const char dht_repl_caps = '!';

void DHT_t::HandleData(std::string & fromaddr, char * buff, size_t bufflen, WriteFunction write) {

  if ( bufflen < 2 ) {
    // short packet
    // drop it
    return;
  }
  char dht_command = buff[1];

  DHT_Key_t target;
  in6_addr target_addr;
  if (bufflen < target.size() + 2 ) {
    // short packet
    // drop it
    return;
  }
  
  // get the request target  
  memcpy(target.data(), buff+2, target.size());
  memcpy(&target_addr.s6_addr, buff+2, sizeof(target_addr.s6_addr));
  
  // pending operations
  DHT_PendingTable_t::iterator itr;
  DHT_Pending_t operation;

  
  // compute the b32 address
  in6_addr remote_addr = i2p_b32addr_t(fromaddr);

  
  // the closest peer
  DHT_Request_t closest;
  // distance between remote and local ipv6 addresses
  int distance = 0;
  std::cerr << (int)dht_command << std::endl;
  switch(dht_command) {

  case dht_req_get: // someone asks us to resolve an address

    // do we know the target address ?
    if ( KnownAddr(target_addr) ) {
      // yes
      // forward the request to that peer
      write(GetDest(target_addr), buff, bufflen);
    } else {
      // no
      // get the closest node and the distance between them and us
      closest = getClosestForAddr(remote_addr);
      // compare the closest node to us
      distance = memcmp(closest.first.data(), node_addr.s6_addr, closest.first.size());
      // can we get closer ?
      if (distance) {
        // yes
        // relay this request
        write(closest.second, buff, bufflen);
        // add this to pending remote requests
        remote_pending.push_back(closest);
      } else {
        // no
        // reject this request because we can't go any farther
        buff[1] = dht_repl_reject;
        write(fromaddr, buff, bufflen);
      }
    }
  case dht_repl_found: // this was previously requested
    // find the pending operation
    operation = getLocalPending(target);
    // we don't have a pending operation
    if (operation == local_pending.end()) {
      // we can't do shit, maybe log but nah.
      break;
    }
    // is it really them ?
    if (memcmp(remote_addr.s6_addr, target.data(), 16) == 0) {
      // yes
      // the operation is done
      local_pending.erase(operation);
      // add them to our routing table
      Put(fromaddr);
    } else {} // it's not them? drop it.
    break;
  case dht_repl_reject: // request rejected
    operation = getRemotePending(target);
    break;
  default: // command not known
    // reject this request
    buff[1] = dht_repl_reject;
    // reply to them
    write(fromaddr, buff, bufflen);
  }
}

bool DHT_t::KnownDest(std::string & destblob) {

  i2p_b32addr_t destaddr(destblob);
  // make ipv6 address for this destblob
  in6_addr addr = destaddr;
  // get closest known address for the address
  auto req = getClosestForAddr(addr);
  // the closest destination is the same if it's known
  return req.second == destblob;
}

bool DHT_t::KnownAddr(in6_addr & addr) {
  DHT_Key_t k1, k2;
  memcpy(k1.data(), node_addr.s6_addr, k1.size());
  memcpy(k2.data(), addr.s6_addr, k2.size());

  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(k1, k2);
  // count bits of the distance to get the index
  uint16_t bits = CountDistanceBits(dist);
  
  auto idx = bits / dht_kbucket_count;
  // this will hang if the routing table is empty
  // make sure that never happens k?
  while(routing_table[idx].empty()) {
    idx = (idx + 1) % dht_kbucket_count;
  }
  return KBucketContainsKey(routing_table[idx], k2);
}

std::string DHT_t::GetDest(in6_addr & addr) {
  auto req = getClosestForAddr(addr);
  // do we know it?
  if (memcmp(req.first.data(), addr.s6_addr, req.first.size())) {
    // no, wtf?
    return "";
  } else {
    // yes, return it
    return req.second;
  }
}

void DHT_t::Find(in6_addr & addr, WriteFunction write) {
  size_t bufflen = 2 + sizeof(addr.s6_addr);
  // build request
  char buff[bufflen];
  buff[0] = dht_byte;
  buff[1] = dht_req_get;
  memcpy(buff+2, addr.s6_addr, sizeof(addr.s6_addr));

  // find closest peer for this address
  auto req = getClosestForAddr(addr);
  std::cerr << "asking "<< addr_tostring(req.first) << std::endl;
  // is this node farther away ?
  if (memcmp(node_addr.s6_addr, req.first.data(), req.first.size())) {
    // yup
    // send it out
    write(req.second, buff, bufflen);
  } else {} // nope? wtf?! drop it.
  
}

DHT_Pending_t DHT_t::getRemotePending(DHT_Key_t & key) {
  auto itr = remote_pending.begin();
  while( itr != remote_pending.end()) {
    if ((*itr).first == key) {
      break;
    }
  }
  return itr;
}
DHT_Pending_t DHT_t::getLocalPending(DHT_Key_t & key) {
  auto itr = local_pending.begin();
  while( itr != local_pending.end()) {
    if ((*itr).first == key) {
      break;
    }
  }
  return itr;
}

void DHT_t::Put(std::string & destblob) {
  if ( KnownDest(destblob) ) {
    std::cerr << "cannot put what is already there" << std::endl;
    return;
  }
  in6_addr addr = i2p_b32addr_t(destblob);
  auto kbucket = getKBucketForAddr(addr);
  // put it into the routing table
  KBucketPut(kbucket, destblob);
}

DHT_KBucket_t & DHT_t::getKBucketForAddr(in6_addr & addr) {
  DHT_Key_t k1, k2;
  memcpy(k1.data(), node_addr.s6_addr, k1.size());
  memcpy(k2.data(), addr.s6_addr, k2.size());

  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(k1, k2);
  // count bits of the distance to get the index
  uint16_t bits = CountDistanceBits(dist);
  // return the bucket
  return routing_table[bits/dht_kbucket_count];
}

DHT_Request_t DHT_t::getClosestForAddr(in6_addr & addr) {
  DHT_Key_t k1, k2;
  memcpy(k1.data(), node_addr.s6_addr, k1.size());
  memcpy(k2.data(), addr.s6_addr, k2.size());

  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(k1, k2);
  // count bits of the distance to get the index
  uint16_t bits = CountDistanceBits(dist);
  
  auto idx = bits / dht_kbucket_count;
  // this will hang if the routing table is empty
  // make sure that never happens k?
  while(routing_table[idx].empty()) {
    idx = (idx + 1) % dht_kbucket_count;
  }
  DHT_Val_t val = KBucketGetClosest(routing_table[idx], k2);
  return {k2, val};
}


DHT_KeyDistance_t ComputeKademlia(DHT_Key_t & k1, DHT_Key_t & k2) {
  DHT_KeyDistance_t dist;
  size_t idx = 0;
  while(idx < dist.size()) {
    dist[idx] = k1[idx] ^ k2[idx];
    ++idx;
  }
  return dist;
}

uint16_t CountDistanceBits(DHT_KeyDistance_t & dist) {
  uint16_t bits = 0;
  for ( auto b : dist ) {
    for (auto i = 0 ; i < 8 ; ++i, b >>= 1) {
      if (b & 0x01 ) {
        ++bits;
      }
    }
  }
  return bits;
}

bool KBucketContainsKey(DHT_KBucket_t & bucket, DHT_Key_t & key) {
  for ( auto & val : bucket ) {
    if ( memcmp(val.first.data(), key.data(), 16) == 0) {
      return true;
    }
  }
  return false;
}

void KBucketPut(DHT_KBucket_t & bucket, DHT_Val_t & val) {
  i2p_b32addr_t addr(val);
  DHT_Key_t k;
  memcpy(k.data(), addr, 16);
  DHT_Request_t req = {k, val};
  bucket.push_back(req);
}

DHT_Val_t KBucketGetClosest(DHT_KBucket_t & bucket, DHT_Key_t &k) {
  int min_idx = 0;
  int idx = 0;
  uint16_t min_dist = 0xffff;
  DHT_Key_t current;
  DHT_KeyDistance_t dist;
  for( auto & item : bucket ) {
    auto addr = item.first;
    dist = ComputeKademlia(k, addr);
    uint16_t bits = CountDistanceBits(dist);
    if (bits < min_dist) {
      min_dist = bits;
      min_idx = idx;
    }
    ++idx;
  }
  return bucket[min_idx].second;
}

#include "dht.hpp"
#include "address.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

#define DHT_FILENAME "nodes.txt"

DHT_t::DHT_t() {
}

DHT_t::~DHT_t() {
}

void DHT_t::Init(std::string & our_dest) {
  if (verbose) {
    std::cerr << "init dht, we are " << our_dest << std::endl;
  }
  node_dest = our_dest;
  // set our node address
  node_addr = i2p_b32addr_t(our_dest);
  Put(our_dest);
  if ( ! KBucketContainsVal(&routing_table[0], our_dest) ) {
    std::cerr << "Init() failed to Put initial address" << std::endl;
  }
}

void DHT_t::Restore(std::string fname) {
  if (verbose) {
    std::cout << "restore cache" << std::endl;
  }
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
  
  // get the reply address
  std::string reply_to(buff+2+target.size());

  DHT_Operation_t operation;
  
  // compute the b32 address
  DHT_Key_t remote;
  in6_addr remote_addr = i2p_b32addr_t(fromaddr);
  memcpy(remote.data(), remote_addr.s6_addr, remote.size());

  // full dht request information
  DHT_PendingRequest_t req = {fromaddr, {target, reply_to}};
  
  // the closest peer
  DHT_Request_t closest;
  DHT_Key_t closest_key;
  DHT_Val_t closest_dest;
  if (verbose) {
    std::cerr << (int)dht_command << std::endl;
  }
  switch(dht_command) {

  case dht_req_get: // someone asks us to resolve an address

    // do we know the target address ?
    if ( KnownAddr(target_addr) ) {
      // yes we know them
      // tell requester that this was accepted
      buff[1] = dht_repl_accept;
      write(fromaddr, buff, bufflen);
      // are we the target?
      if ( memcmp(target_addr.s6_addr, node_addr.s6_addr, sizeof(target_addr.s6_addr)) == 0) {
        // ya we are the target
        // tell reply_to destination about us
        buff[1] = dht_repl_found;
        write(reply_to, buff, bufflen);
      } else {
        // we are not the target but we know it
        // forward the request to the peer that is the target
        buff[1] = dht_req_get;
        std::string dest = GetDest(target_addr);
        write(dest, buff, bufflen);
      }
    } else {
      // no we don't know them
      // get the closest node and the distance between that node and us
      closest = getClosestForAddr(remote_addr);
      
      // are we the closest to this?
      if (memcmp(closest.first.data(), node_addr.s6_addr, 16) == 0) {
        // yes we are closest and we can't get closer
        // reject this request because we can't go any farther
        buff[1] = dht_repl_reject;
        write(fromaddr, buff, bufflen);
      } else {
        // we are not the closet but we can get closer
        // is this a newly requested operation ?

        if (std::find(std::begin(remote_pending), std::end(remote_pending), req) == std::end(remote_pending)) {
          // didn't find a pending remote operation
          // let's add it
          remote_pending.push_back(req);
        } else {} // it's probably a repeat, process anyways
        // tell requester the request was accepted
        buff[1] = dht_repl_accept;
        write(fromaddr, buff, bufflen);
        // relay this request to the closer node
        buff[1] = dht_req_get;
        write(closest.second, buff, bufflen);  
      }
    }
    break;
  case dht_repl_found: // this was previously requested
    // find the pending operation
    operation = std::find(std::begin(local_pending), std::end(local_pending), req);
    if (operation == std::end(local_pending) ) {
      // didn't find local operation
      // we didn't ask for it? or stuff?
      // complain
      std::cerr << "unwarrented dht_repl_found from " << addr_tostring(remote_addr);
      std::cerr << std::endl;
    } else if (memcmp(remote_addr.s6_addr, target.data(), 16) == 0) {
      // it's really who we think it is
      // the operation is done
      local_pending.erase(operation);
      // add them to our routing table
      Put(fromaddr);
    } else {
      // it's not who they say it is huh?
      // complain
      std::cerr << "spoofed dht_repl_found from " << addr_tostring(remote_addr);
      std::cerr << std::endl;
    }
    break;
  case dht_repl_reject: // request rejected
    operation = std::find(std::begin(local_pending), std::end(local_pending), req);

    // did we request this?
    if (operation == std::end(local_pending)) {
      // nope we did NOT request it
      // did someone else request it?
      operation = std::find(std::begin(remote_pending), std::end(remote_pending), req);
      if (operation == std::end(remote_pending)) {
        // nope, wtf? unwarrented dht_repl_reject
        // complain
        std::cerr << "unwarrented dht_repl_reject from " << addr_tostring(remote_addr);
        std::cerr << std::endl;
        break;
      }
      
    }
    
    // do we have anyone closer?
    if (getNextClosest(target, &closest_key)) {
      // ya we have a closer peer, get it
      closest_dest = GetDest(closest_key);
      // ask them instead
      buff[1] = dht_req_get;
      write(closest_dest, buff, bufflen);
    } else {
      // nope we don't have any closer peers
      // can't do shit so complain
      if (verbose) {
        std::cerr << "failed to resolve " << addr_tostring(target);
        std::cerr << " cannot get closer" << std::endl;
      }
      // is this a locally requested operation?
      operation = std::find(std::begin(local_pending), std::end(local_pending), req);
      if (operation != std::end(local_pending)) {
        // ya, remove it
        local_pending.erase(operation);
      } else {
        // is this a remotely requested operation?
        operation = std::find(std::begin(remote_pending), std::end(remote_pending), req);
        if (operation != std::end(remote_pending)) {
          // ya, it's remotely requested

          // tell the person that requseted this it failed
          buff[1] = dht_repl_reject;
          write(req.first, buff, bufflen);
          // remove it
          remote_pending.erase(operation);
        } else {
          // nope? we don't know where this reject came from.
          // complain
          std::cerr << "unwarrented dht_repl_reject from " << addr_tostring(target);
          std::cerr << std::endl;
        }
      }
    }
    break;
  case dht_repl_accept: // previous request was accepted

    // is this a local request?
    operation = std::find(std::begin(local_pending), std::end(local_pending), req);
    if (operation != std::end(local_pending)) {
      // yes it's local
      local_pending.erase(operation);
    } else {
      operation = std::find(std::begin(remote_pending), std::end(remote_pending), req);
      // is it remote?
      if (operation == std::end(remote_pending)) {
        // yas, it's pending remote request
        // remove it
        remote_pending.erase(operation);
      } else {
        // we don't have this request pending locally or remotely
        // complain
        std::cerr << "unwarrented dht_repl_accept for " << addr_tostring(target);
        std::cerr << " from " << addr_tostring(remote_addr);
        std::cerr << std::endl;
      }
    }
    break;
  default: // command not known
    // we don't know wtf they want so reject
    buff[1] = dht_repl_reject;
    // reply to them
    write(fromaddr, buff, bufflen);
  }
}

bool DHT_t::KnownDest(std::string & destblob) {
  if (verbose) {
    std::cerr << "KnownDest() checking for " << destblob << std::endl;
  }
  DHT_Val_t dest = destblob;
  auto kbucket = getKBucketForDest(dest);
  bool has = KBucketContainsVal(kbucket, destblob);
  if (verbose) {
    std::cerr << "KnownDest() has dest " << has << std::endl;
  }
  return has;
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
  if (verbose) {
    std::cerr << "KnownAddr() checking bucket " << idx << std::endl;
  }
  bool has = KBucketContainsKey(&routing_table[idx], k2);
  if ( verbose ) {
    std::cerr << "KnownAddr() contains val? " << has << std::endl;
  }
  return has;
}

std::string DHT_t::GetDest(in6_addr & addr) {
  auto req = getClosestForAddr(addr);
  // do we know it?
  if (memcmp(req.first.data(), addr.s6_addr, req.first.size())) {
    // no, wtf?
    throw std::logic_error("DHT::GetDest() failed");
  } else {
    // yes, return it
    return req.second;
  }
}

std::string DHT_t::GetDest(DHT_Key_t & k) {
  in6_addr addr = k;
  return GetDest(addr);
}

void DHT_t::Find(in6_addr & addr, WriteFunction write) {
  // dht byte + request type + target address + destination length
  size_t bufflen = 2 + sizeof(addr.s6_addr) + node_dest.size() + 1;
  // build request
  char buff[bufflen];
  // make this a dht request
  buff[0] = dht_byte;
  // dht get request
  buff[1] = dht_req_get;
  // target address
  memcpy(buff+2, addr.s6_addr, sizeof(addr.s6_addr));
  // our dest
  memcpy(buff+2+sizeof(addr.s6_addr), node_dest.c_str(), node_dest.size());
  // null terminate
  buff[bufflen-1] = 0;
  // find closest peer for this address
  auto req = getClosestForAddr(addr);
  if (req.second == node_dest) {
    // the cloesest node is us?
    // wtf we can't do shit
    std::cerr << "OUT OF PEERS! We don't know anyone near " << addr_tostring(addr);
    std::cerr << std::endl;
    return;
  }
  if (verbose) {
    std::cerr << "asking "<< addr_tostring(req.first) << std::endl;
  }
  // is this node farther away ?
  if (memcmp(node_addr.s6_addr, req.first.data(), req.first.size())) {
    // yup
    // send it out
    write(req.second, buff, bufflen);
  } else {} // nope? wtf?! drop it.
  
}

DHT_RequestHistory_t * DHT_t::getRequestHistoryForAddr(DHT_Key_t & k) {
  auto itr = request_history.find(k);
  // do we have request history for this key?
  if (itr == request_history.end()) {
    // nope
    // let's create the history
    request_history[k] = DHT_RequestHistory_t();
  }
  return &request_history.at(k);
}


void DHT_t::Put(std::string & destblob) {
  if ( KnownDest(destblob) ) {
    std::cerr << "Put() failed, entry already known" << std::endl;
    return;
  }
  DHT_Val_t dest = destblob;
  auto kbucket = getKBucketForDest(dest);
  // put it into the routing table
  KBucketPut(kbucket, dest);
  if ( verbose ) {
    std::cerr << "Put() suceess: " <<  KnownDest(destblob) << std::endl;
  }
}

DHT_KBucket_t * DHT_t::getKBucketForDest(DHT_Val_t & dest) {
  i2p_b32addr_t addr(dest);
  DHT_Key_t k1, k2;
  memcpy(k1.data(), node_addr.s6_addr, k1.size());
  memcpy(k2.data(), addr, k2.size());

  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(k1, k2);
  // count bits to get the kbucket index
  uint16_t bits = CountDistanceBits(dist);
  // we got the index
  int idx = bits / dht_kbucket_count;
  if (verbose) {
    std::cerr << "KBucketForDest() get bucket " << idx << std::endl;
  }
  return &routing_table[idx];
}

DHT_KBucket_t * DHT_t::getKBucketForAddr(in6_addr & addr) {
  DHT_Key_t k1, k2;
  memcpy(k1.data(), node_addr.s6_addr, k1.size());
  memcpy(k2.data(), addr.s6_addr, k2.size());

  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(k1, k2);
  // count bits of the distance to get the kbucket index
  uint16_t bits = CountDistanceBits(dist);
  // return the bucket
  int idx = bits/dht_kbucket_count;
  if (verbose) {
    std::cerr << "kbucketForAddr() get bucket " << idx << std::endl;
  }
  return &routing_table[idx];
}

DHT_Request_t DHT_t::getClosestForAddr(in6_addr & addr) {
  DHT_Key_t k1, k2;
  memcpy(k1.data(), node_addr.s6_addr, k1.size());
  memcpy(k2.data(), addr.s6_addr, k2.size());

  if (verbose) {
    std::cerr << "get closest for " << (std::string) k2 << std::endl;
  }
  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(k1, k2);
  // count bits of the distance to get the index
  uint16_t bits = CountDistanceBits(dist);
  
  auto idx = bits / dht_kbucket_count;
  if (verbose) {
    std::cerr << "closestForAddr() get bucket " << idx << std::endl;
  }
  // this will hang if the routing table is empty
  // make sure that never happens, k?
  while(routing_table[idx].empty()) {
    idx = (idx + 1) % dht_kbucket_count;
  }
  return KBucketGetClosest(&routing_table[idx], k2);
}

bool DHT_t::getNextClosest(DHT_Key_t & target, DHT_Key_t * result) {
  // get our history
  auto history = getRequestHistoryForAddr(target);

  DHT_Key_t our_key;
  memcpy(our_key.data(), node_addr.s6_addr, our_key.size());
  DHT_Request_t req;

  if (verbose) {
    std::cerr << "getNextClosest() " << (std::string) our_key << std::endl;
  }
  
  // compute distance between them and me
  DHT_KeyDistance_t dist = ComputeKademlia(our_key, target);
  // count bits of the distance to get the index
  uint16_t bits = CountDistanceBits(dist);
  
  auto idx = bits / dht_kbucket_count;
  
    
  do {
    // get the current kbucket we are looking in
    auto kbucket = &routing_table[idx];
    if (verbose) {
      std::cerr << "getNextClosest() try bucket " << idx << std::endl;
    }
    // did we find something in this kbucket that isn't in our history?
    if (KBucketGetClosestExclude(kbucket, history, target, &req)) {
      // yes we did find something
      // return that we found it
      *result = req.first;
      return true;
    } else {
      // we didn't find it in this bucket, try the next one
      ++idx;
    }
    // keep on going until we can't find anything closer
  } while(idx < dht_kbucket_count);
  // we didn't find shit!
  return false;
  
}

DHT_RequestHistory_t * RequestHistoryGet(DHT_RequestTable_t * req_table, DHT_Key_t & target) {
  // find existing history
  auto itr = req_table->find(target);
  // did we find it?
  if (itr == req_table->end()) {
    // nope, we didn't find it
    // we should create the history
    DHT_RequestHistory_t history;
    (*req_table)[target] = history;
    // return the newly created history
    return &(*req_table)[target];
  } else {
    // yup, we found it
    // return the existing history
    return &itr->second;
  }

}

void RequestHistoryPut(DHT_RequestTable_t * req_table, DHT_Key_t & target, DHT_Key_t & tried) {
  auto history = RequestHistoryGet(req_table, target);
  // do we have this entry in our history already?
  for ( auto & val : *history ) {
    if (val == tried ) {
      // yes it's in our history, wtf?
      // complain about it
      std::string target_str = target;
      std::string tried_str = tried;
      std::cerr << "RequestHistoryPut() already have entry in our history: target=" << target_str << " tried=" << tried_str;
      std::cerr << std::endl;
      return;
    }
  }
  // nope
  // add it
  history->push_back(tried);
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
  for ( uint8_t b : dist ) {
    for (auto i = 0 ; i < 8 ; ++i) {
      if ((b & 0x01 )== 0x01) {
        ++bits;
      }
      b = b >> 1;
    }
  }
  return bits;
}

bool KBucketContainsKey(DHT_KBucket_t * bucket, DHT_Key_t & key) {
  for ( auto & val : *bucket ) {
    if ( memcmp(val.first.data(), key.data(), 16) == 0) {
      return true;
    }
  }
  return false;
}

bool KBucketContainsVal(DHT_KBucket_t * bucket, DHT_Val_t & val) {
  std::string val1 = val;
  for ( DHT_Request_t & item : *bucket ) {
    std::string val2 = item.second;
    if ( val2 == val1 ) {
      return true;
    }
  }
  return false;
}

void KBucketPut(DHT_KBucket_t * bucket, DHT_Val_t & val) {
  i2p_b32addr_t addr(val);
  DHT_Key_t k;
  memcpy(k.data(), addr, 16);
  DHT_Request_t req = {k, val};
  bucket->push_back(req);
}

bool RequestHistoryContains(DHT_RequestHistory_t * history, DHT_Key_t & k) {
  for ( auto & item : *history) {
    if (item == k) { return true; }
  }
  return false;
}



DHT_Request_t KBucketGetClosest(DHT_KBucket_t * bucket, DHT_Key_t &k) {
  DHT_Request_t req;
  KBucketGetClosestExclude(bucket, nullptr, k, &req);
  return req;
}

bool KBucketGetClosestExclude(DHT_KBucket_t * bucket, DHT_RequestHistory_t * history, DHT_Key_t &k, DHT_Request_t * result) {
  int min_idx = 0;
  int idx = 0;
  uint16_t min_dist = 0xffff;
  DHT_KeyDistance_t dist;
  bool found = false;
  for( auto & item : *bucket ) {
    auto addr = item.first;
    // check exclude list
    if ( history != nullptr && RequestHistoryContains(history, addr) ) {
      continue;
    }
    // we have an entry that isn't on the exclude list
    found = true;
    // compute distance
    dist = ComputeKademlia(k, addr);
    uint16_t bits = CountDistanceBits(dist);
    // check for local minimum
    if (bits < min_dist) {
      min_dist = bits;
      min_idx = idx;
    }
    ++idx;
  }
  // did we get a result?
  if (found) {
    // ya, put it in there
    *result = bucket->at(min_idx);
  }
  return found;
}

#ifndef ADDRESS_HPP
#define ADDRESS_HPP
#include <string>
#include <netinet/in.h>
//
// samtun ipv4 address deriviation
// copywrong you're mom 2015
//


// compute the ipv6 address for this destination blob
// uses 0200::/8 address block (supposivly deprecated)
// The next 15 bytes are bytes 1 to 16 from the sha256
// of the unpacked base64 destination blob.
void compute_address(std::string destination, in6_addr * addr);

// return true if we know this destination's ipv6 address
bool cached_destination(std::string destination);

// remember that this destination maps to a ipv6 address
void save_endpoint(std::string destination);

// check our local destination cache for a destination
// mapped to this address
// return empty string on fail
std::string lookup_destination(in6_addr addr);

// convert an ipv6 address to string
std::string addr_tostring(in6_addr addr);

// save the cache to disk
void persist_cache();

// restore the cache from disk
void restore_cache();

#endif

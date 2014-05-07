#ifndef I2TUN_INET_HPP
#define I2TUN_INET_HPP
#include <netinet/in.h>
#include <vector>

namespace i2tun 
{

    constexpr uint16_t MTU = 1500;

    typedef in6_addr inet_address_t;

    typedef std::vector<uint8_t> inet_packet_t;

}

#endif

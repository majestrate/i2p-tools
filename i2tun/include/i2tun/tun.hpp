#ifndef I2TUN_TUN_HPP
#define I2TUN_TUN_HPP
#include <i2tun/inet.hpp>
#include <string>

namespace i2tun 
{
    typedef int tunfd_t;

    /**
       Platform Independant ip tun device
     */
    class TunDevice
    {
    public:
        TunDevice(std::string const & ifname);
        virtual ~TunDevice();
        
        void send_l3(inet_address_t & src, inet_address_t & dst, inet_packet_t & data);
        void send_l3(inet_address_t & src, inet_address_t & dst, void * data, size_t dlen); 

        inet_address_t get_if_addr();
        void set_if_addr(inet_address_t & addr);

    private:
        tunfd_t tunfd;
        inet_address_t _our_addr;
    };
    
    
}

#endif

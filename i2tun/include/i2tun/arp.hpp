#ifndef I2TUN_ARP_HPP
#define I2TUN_ARP_HPP
#include <i2tun/inet.hpp>
#include <i2tun/i2p.hpp>
#include <tuple>

namespace i2tun 
{
    
    typedef std::tuple<inet_address_t, i2p_address_t> i2tun_mapping_t;

    class AddressMapper
    {
    public:
        AddressMapper();
        ~AddressMapper();

        void load_mapping(std::string const & fname);
        void clear_mapping();
        void save_mapping(std::string const & fname);

        bool has_inet(inet_address_t & addr);
        bool has_dest(i2p_address_t & dest);

        void map(i2tun_mapping_t & mapping);
        void unmap(i2tun_mapping_t & mapping);

        bool dest(i2tun_mapping_t & mapping);
        bool inet(i2tun_mapping_t & mapping);

    };
}

#endif

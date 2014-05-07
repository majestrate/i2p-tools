#include <i2tun/i2tun.hpp>
#include <i2cp.hpp>

#include <iostream>

int main(int argc, char * argv[])
{
    i2cp::I2CP _;
    {
        i2cp::client client;
        client.connect();
        if (client.is_connected()) {
            i2cp::destination dest;
            std::cout << dest;
        } else {
            std::cout << "we are not connected";
        }
        std::cout << std::endl;
    }
}


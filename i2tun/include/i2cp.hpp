#ifndef I2CP_HPP
#define I2CP_HPP
#include <gmp.h>
#include <iostream>

namespace i2cp 
{
    namespace capi 
    {
        extern "C" {
#include <i2cp/i2cp.h>
        }
    }



    class destination
    {

    public:
        
        destination() : m_dest(capi::i2cp_destination_new()) {}

        destination(capi::i2cp_destination_t * dest) :
            m_dest(dest) {}

        ~destination() { if ( m_dest != nullptr ) { capi::i2cp_destination_destroy(m_dest); } }

        std::string str () { 
            if( m_dest == nullptr) { return "<NULL>"; }
            else { return capi::i2cp_destination_b32(m_dest); }
        }

    private:
        capi::i2cp_destination_t * m_dest;
        
    };


    std::ostream & operator << (std::ostream & os, destination & dest) {
        os << dest.str();
        return os;
    }

    

    class client 
    {
    public:
        client() : 
            m_client(capi::i2cp_client_new(&m_hooks)),
            m_hooks({nullptr, nullptr, nullptr})
            {}
                        
        ~client() {
            if (is_connected()) { disconnect(); }
            capi::i2cp_client_destroy(m_client);
        }

        void disconnect() {
            capi::i2cp_client_disconnect(m_client);
        }
        
        void connect() {
            capi::i2cp_client_connect(m_client);
        }

        bool is_connected() {
            return capi::i2cp_client_is_connected(m_client) == 1;
        }

        int tick() {
            return capi::i2cp_client_process_io(m_client);
        }

        destination lookup(const std::string & name) {
            return destination();
        }

    private:
        capi::i2cp_client_t * m_client;
        capi::i2cp_client_callbacks_t m_hooks;
    };

    class I2CP
    {
    public:
        I2CP() { capi::i2cp_init(); }
        ~I2CP() { capi::i2cp_deinit(); }
    };

}
        
#endif
        

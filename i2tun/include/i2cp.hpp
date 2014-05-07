#ifndef I2CP_HPP
#define I2CP_HPP
#include <gmp.h>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <chrono>
#include <iostream>

namespace i2cp 
{
    namespace capi 
    {
        extern "C" {
#include <i2cp/i2cp.h>
        }
    }

    class stream 
    {
    public:
        stream(size_t size=0xffff) {
            m_stream.data = (uint8_t *) malloc(size);
            capi::bzero(m_stream.data, size);
            m_stream.size = size;
            reset();
        }

        ~stream() { stream_destroy(&m_stream); }
        
        void reset() {
            stream_reset(&m_stream);
        }
        
        void put(void * data, size_t size) {
            reset();
            stream_check(&m_stream, size);
            memcpy(&m_stream, data, size);
            m_stream.p += size;
        }

        capi::stream_t * ptr() {
            return &m_stream;
        }
        
    private:
        capi::stream_t m_stream;
    };

 

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

    class datagram
    {
    public:
        datagram(capi::i2cp_destination_t * dest, capi::i2cp_datagram_t * ptr):  m_dgram(ptr), from(dest) {}
        datagram(capi::i2cp_datagram_t * ptr) : m_dgram(ptr) {}
        ~datagram() { if (m_dgram != nullptr) { capi::i2cp_datagram_destroy(m_dgram); } }

        void set(void * data, size_t dlen) {
            stream st(dlen);
            st.put(data,dlen);
            capi::i2cp_datagram_set_payload(ptr(), st.ptr());
        }

        capi::i2cp_datagram_t * ptr() { return m_dgram; }
    private:
        capi::i2cp_datagram_t * m_dgram;
        destination from;
    };


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
        
        capi::i2cp_client_t * ptr() { return m_client; }

    private:
        capi::i2cp_client_t * m_client;
        capi::i2cp_client_callbacks_t m_hooks;
    };

    void session_on_message(capi::i2cp_session_t * _, capi::i2cp_protocol_t proto,
                            uint16_t srcport, uint16_t dstport, capi::stream_t * payload,
                            void * sess);

    void session_on_status(capi::i2cp_session_t * _, capi::i2cp_session_status_t status,
                            void * sess);

    void session_on_destination(capi::i2cp_session_t * _, uint32_t rid, const char * addr,
                                capi::i2cp_destination_t * dest, void * sess);

    class session
    {
    public:

        session(capi::i2cp_client_t * cl) : session(cl, nullptr) {}
        session(capi::i2cp_client_t * cl, const std::string & fname) : session(cl, fname.c_str()) {}


        virtual void on_message(capi::i2cp_protocol_t proto,
                                uint16_t src_port, uint16_t dst_port, capi::stream_t * payload) {}
        
        void _on_status(capi::i2cp_session_status_t status) {
            switch (status) {
            case capi::I2CP_SESSION_STATUS_CREATED:
                created();
                break;
            default:
                on_status(status);
                break;
            }
        }

        virtual void created() {}

        virtual void on_status(capi::i2cp_session_status_t status) {}

        virtual void on_destination(uint32_t rid, const std::string & addr, capi::i2cp_destination_t * dest) {}

        
        void recv(datagram & dgram) {
            //TODO: implement
        }

        ~session() { if ( m_session != nullptr ) { capi::i2cp_session_destroy(m_session); } }

    private:
        session(capi::i2cp_client_t * i2cp_client, const char * fname) {
            m_hooks.opaque = this;
            m_hooks.on_message = &session_on_message;
            m_hooks.on_status = &session_on_status;
            m_hooks.on_destination = &session_on_destination;
            m_session = capi::i2cp_session_new(i2cp_client, &m_hooks, fname);
        }

        capi::i2cp_session_t * m_session;
        capi::i2cp_session_callbacks_t m_hooks;
    };


    void session_on_message(capi::i2cp_session_t * _, capi::i2cp_protocol_t proto,
                            uint16_t srcport, uint16_t dstport, capi::stream_t * payload,
                            void * sess) 
    {
        ((session *)sess)->on_message(proto, srcport, dstport, payload);
    }


    void session_on_status(capi::i2cp_session_t * _, capi::i2cp_session_status_t status,
                            void * sess)
    {
        ((session *)sess)->_on_status(status);
    }

    void session_on_destination(capi::i2cp_session_t * _, uint32_t rid, const char * addr,
                                capi::i2cp_destination_t * dest, void * sess)
    {
        ((session *)sess)->on_destination(rid, std::string(addr), dest);
    }


    class I2CP
    {
    public:
        I2CP() { capi::i2cp_init(); }
        ~I2CP() { capi::i2cp_deinit(); }
    };

}
        
#endif
        

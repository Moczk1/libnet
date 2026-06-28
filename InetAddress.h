#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

#include <strings.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

namespace m_libnet
{
    class InetAddress
    {
    private:
        /* data */
        sockaddr_in m_addr;

    public:
        explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
        explicit InetAddress(const sockaddr_in &addr) : m_addr(addr) {};

        const sockaddr_in *getSockAddr() const { return &m_addr; };
        void setSockAddr(const sockaddr_in &addr) { m_addr = addr; }

        std::string toIp() const;
        std::string toIpPort() const;
        uint64_t toPort() const;
    };

    
} // namespace m_netlib

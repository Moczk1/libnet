#include "InetAddress.h"

namespace m_libnet
{
    InetAddress::InetAddress(uint16_t port, std::string ip)
    {
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = ::htons(port);
        m_addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
    }
    std::string InetAddress::toIp() const
    {
        char buf[64] = {0};
        ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof buf);
        return buf;
    }
    std::string InetAddress::toIpPort() const
    {
        char buf[64] = {0};
        ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof buf);
        size_t end = strlen(buf);
        uint64_t port = ::ntohs(m_addr.sin_port);
        sprintf(buf + end, ":%u", port);
        return buf;
    }
    uint64_t InetAddress::toPort() const
    {
        return ::ntohs(m_addr.sin_port);
    }
} // namespace m_libnet

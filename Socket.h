#pragma once


#include "Logger.h"
#include "InetAddress.h"

namespace m_libnet
{
    class Socket
    {
    private:
        const int m_fd;

    public:
        explicit Socket(int fd) : m_fd(fd) {}
        ~Socket() { close(m_fd); }
        const int fd() const { return m_fd; }

        void bindAddress(const InetAddress &localaddr);
        void startListen();
        int accpet(InetAddress *peeraddr);

        void shutdownWrite();
        void setTcpNoDelay(bool on);
        void setReuseAddr(bool on);
        void setReusePort(bool on);
        void setKeepAlive(bool on);
    };
} // namespace m_libnet

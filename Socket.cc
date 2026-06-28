#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Logger.h"
#include "Socket.h"

namespace m_libnet
{
    void Socket::bindAddress(const InetAddress &localaddr)
    {
        if (0 != ::bind(m_fd, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)) < 0)
        {
            LOG_FATAL("bind sockfd:%d fail\n", m_fd);
        }
    }
    void Socket::startListen()
    {
        if (0 != ::listen(m_fd, 1024) < 0)
        {
            LOG_FATAL("listen sockfd:%d fail\n", m_fd);
        }
    }
    int Socket::accpet(InetAddress *peeraddr)
    {
        sockaddr_in addr;
        socklen_t len = sizeof addr;
        ::memset(&addr, 0, sizeof addr);

        // 获得系统分配的本地通信的 fd;
        int connfd = ::accept4(m_fd, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (connfd > 0)
        {
            peeraddr->setSockAddr(addr);
        }

        return connfd;
    }

    void Socket::shutdownWrite()
    {
        if (::shutdown(m_fd, SHUT_WR) < 0)
        {
            LOG_ERROR("shutdownWrite error");
        }
    }
    void Socket::setTcpNoDelay(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof opt);
    }
    void Socket::setReuseAddr(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    }
    void Socket::setReusePort(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof opt);
    }
    void Socket::setKeepAlive(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof opt);
    }
} // namespace m_libnet

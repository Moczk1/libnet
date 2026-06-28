#pragma once

#include <functional>
#include "Socket.h"
#include "Channel.h"
#include "Accepter.h"

namespace m_libnet
{
    int createFd()
    {
        int lisnfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if (lisnfd < 0)
        {
            LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        }
        return lisnfd;
    }

    Accepter::Accepter(EventLoop *loop, const InetAddress &inetAddress, bool reusePort)
        : m_loop(loop), m_acceptSocket(createFd()), m_acceptChannel(loop, m_acceptSocket.fd())
    {
    }

    Accepter::~Accepter()
    {
        m_acceptChannel.disableAll(); // 清空注册时间
        m_acceptChannel.remove();     // 从 epoll 结构体中删除
    }

    void Accepter::listen()
    {
        m_isListening = true;
        m_acceptSocket.startListen();
        m_acceptChannel.enableReading();
    }

    void Accepter::handleRead()
    {
        InetAddress peerAddr;
        int conctfd = m_acceptSocket.accpet(&peerAddr); // 新进来一个连接建立请求
        if (conctfd >= 0)
        {
            if (m_cb)
            {
                m_cb(conctfd, peerAddr);
            }
            else
            {
                ::close(conctfd);
            }
        }
        else
        {
            LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
            if (errno == EMFILE)
            {
                LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
            }
        }
    }
}
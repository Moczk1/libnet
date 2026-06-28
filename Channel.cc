#include "Channel.h"

namespace m_libnet
{
    Channel::Channel(EventLoop *loop, int fd)
        : m_fd(fd), m_events(NONE), m_retEvents(NONE), m_status(JustCreate), m_loop(loop), m_tied(false)
    {
    }
    void Channel::tie(const std::shared_ptr<void> &obj)
    {
        m_tie = obj;
        m_tied = true;
    }

    void Channel::handleEvent(TimePoint receiveTime)
    {
        if (m_tied)
        {
            std::shared_ptr<void> guard = m_tie.lock();
            if (guard)
            {
                handleEventWithGuard(receiveTime);
            }
        }
        else
        {
            handleEventWithGuard(receiveTime);
        }
    }

    void Channel::handleEventWithGuard(TimePoint receiveTime)
    {
        LOG_INFO("channel handleEvent revents:%d\n", m_retEvents);
        // 关闭
        if ((m_retEvents & EPOLLHUP) && !(m_retEvents & EPOLLIN))
        // 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
        {
            if (m_closeCallBack)
            {
                m_closeCallBack();
            }
        }
        // 错误
        if (m_retEvents & EPOLLERR)
        {
            if (m_errorCallBack)
            {
                m_errorCallBack();
            }
        }
        // 读
        if (m_retEvents & (EPOLLIN | EPOLLPRI))
        {
            if (m_readCallBack)
            {
                m_readCallBack(receiveTime);
            }
        }
        // 写
        if (m_retEvents & EPOLLOUT)
        {
            if (m_writeCallBack)
            {
                m_writeCallBack();
            }
        }
    }
}

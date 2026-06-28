#include <errno.h>
#include <unistd.h>
#include <vector>
#include <sys/epoll.h>
#include <string.h>
#include "Poller.h"
#include "Logger.h"
#include "Channel.h"



namespace m_libnet
{
    EPollPoller::EPollPoller(EventLoop *loop)
        : Poller(loop), m_epollfd(::epoll_create1(EPOLL_CLOEXEC)), m_events(InitEventListSize)
    {
        if (m_epollfd < 0)
        {
            LOG_FATAL("epoll_create error:%d \n", errno);
        }
    }
    EPollPoller::~EPollPoller()
    {
        ::close(m_epollfd);
    }

    std::chrono::system_clock::time_point EPollPoller::poll(int timeoutMs, std::vector<Channel *> &activeChannels)
    {

        LOG_DEBUG("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());
        int retEvents = epoll_wait(m_epollfd, &*m_events.begin(), m_events.size(), timeoutMs);

        int saveErrno = errno;

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        if (retEvents > 0)
        {
            LOG_INFO("%d events happend\n", retEvents); // LOG_DEBUG最合理

            for (int i = 0; i < retEvents; i++)
            {
                Channel *channel = static_cast<Channel *>(m_events[i].data.ptr);
                channel->setRetEvents(m_events[i].events);
                activeChannels.push_back(channel);
            }

            if (retEvents == m_events.size())
            {
                m_events.resize(m_events.size() * 2);
            }
        }
        else if (retEvents == 0)
        {
            LOG_DEBUG("%s timeout!\n", __FUNCTION__);
        }
        else
        {
            if (saveErrno != EINTR)
            {
                errno = saveErrno;
                LOG_ERROR("EPollPoller::poll() error!");
            }
        }
        return now;
    }

    // channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
    void EPollPoller::updateChannel(Channel *channel)
    {
        const Channel::Status status = channel->getStatus();
        LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), status);

        if (status == Channel::Status::JustCreate || status == Channel::Status::Secede)
        {
            if (status == Channel::Status::JustCreate)
            {
                int fd = channel->fd();
                m_channels[fd] = channel;
            }
            else
            {
            }
            channel->setStatus(Channel::Status::AddedInLoop);
            update(EPOLL_CTL_ADD, channel);
        }
        else
        {
            int fd = channel->fd();
            if (channel->isNoneEvent())
            {
                update(EPOLL_CTL_DEL, channel);
                channel->setStatus(Channel::Status::Secede);
            }
            else
            {
                update(EPOLL_CTL_MOD, channel);
            }
        }
    }

    // 从Poller中删除channel
    void EPollPoller::removeChannel(Channel *channel)
    {
        int fd = channel->fd();
        m_channels.erase(fd);
        LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

        Channel::Status status = channel->getStatus();
        if (status == Channel::Status::AddedInLoop)
        {
            update(EPOLL_CTL_DEL, channel);
        }
        channel->setStatus(Channel::Status::JustCreate);
    }

    // 更新channel通道 其实就是调用epoll_ctl add/mod/del
    void EPollPoller::update(int operation, Channel *channel)
    {
        epoll_event packed;
        ::memset(&packed, 0, sizeof packed);

        packed.events = channel->events();
        packed.data.ptr = channel;
        if (::epoll_ctl(m_epollfd, operation, channel->fd(), &packed) < 0)
        {
            if (operation == EPOLL_CTL_DEL)
            {
                LOG_ERROR("epoll_ctl del error:%d\n", errno);
            }
            else
            {
                LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
            }
        }
    }
}

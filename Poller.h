#pragma once
#include <unistd.h>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>

namespace m_libnet
{
    class EventLoop;
    class Channel;
    class Poller
    {
    protected:
        EventLoop *m_loop;
        std::vector<Channel *> m_activeChannels;
        using ChannelMap = std::unordered_map<int, Channel *>;
        ChannelMap m_channels;

    public:
        Poller(EventLoop *loop) : m_loop(loop)
        {
        }
        virtual ~Poller() = default;

        virtual std::chrono::system_clock::time_point poll(int timeoutMs, std::vector<Channel *> &activeChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        virtual void removeChannel(Channel *channel) = 0;
    };

    class EPollPoller : public Poller
    {
    private:
        static const int InitEventListSize = 16;

        int m_epollfd;                     // epoll_create创建返回的fd保存在epollfd_中
        std::vector<epoll_event> m_events; // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集

        // 填写活跃的连接
        // void fillActiveChannels(int numEvents, std::vector<Channel *> &activeChannels) const;
        // 更新channel通道 其实就是调用epoll_ctl
        void update(int operation, Channel *channel);

    public:
        EPollPoller(EventLoop *loop);
        ~EPollPoller() override;

        std::chrono::system_clock::time_point poll(int timeoutMs, std::vector<Channel *> &activeChannels) override;
        void updateChannel(Channel *channel) override;
        void removeChannel(Channel *channel) override;
    };

} // namespace m_libnet

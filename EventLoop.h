#pragma once
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>
#include <sys/types.h>
#include "Logger.h"
#include "Poller.h"

namespace m_libnet
{
    class Channel;
    class EPollPoller;


    class EventLoop
    {
    private:
        /* data */
        std::atomic_bool m_isLooping;
        std::atomic_bool m_hasQuit;

        const pid_t m_pid;

        std::chrono::system_clock::time_point m_pollReturnTime;
        std::unique_ptr<Poller> m_poller;

        int m_wakeupFd;
        std::unique_ptr<Channel> m_wakeupChannel;

        std::vector<Channel *> m_activeChannels;
        std::atomic_bool m_callingPendingFunctors;

        std::vector<std::function<void()>> m_pendingFunctors;
        std::mutex m_mutex;

        inline static int M_POLL_TIMEMS = 10000;

    public:
        EventLoop(/* args */);
        ~EventLoop();

        void loop();
        void quit();

        void runInLoop(std::function<void()> cb);
        void queueInLoop(std::function<void()> cb);

        void wakeup();

        void removeChannel(Channel *);
        void updateChannel(Channel *);

        bool isInLoopThread() const { return m_pid == static_cast<pid_t>(::gettid()); }
        std::chrono::system_clock::time_point pollReturnTime() const { return m_pollReturnTime; }

        void handleRead();        // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
        void doPendingFunctors(); // 执行上层回调
    };

    
} // namespace m_libnet

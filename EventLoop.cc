#include "EventLoop.h"
#include "Channel.h"

namespace m_libnet
{
    thread_local EventLoop *t_loopInThisThread = nullptr;

    EventLoop::EventLoop()
        : m_isLooping(false), m_hasQuit(false), m_callingPendingFunctors(false), m_pid(::gettid())
    {
        m_wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (m_wakeupFd < 0)
        {
            LOG_FATAL("eventfd error:%d\n", errno);
        }
        m_wakeupChannel = std::make_unique<Channel>(this, m_wakeupFd);

        m_poller = std::make_unique<EPollPoller>(this);
        LOG_DEBUG("EventLoop created %p in thread %d\n", this, m_pid);

        if (t_loopInThisThread)
        {
            LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, m_pid);
        }
        else
        {
            t_loopInThisThread = this;
        }

        m_wakeupChannel->setReadCallBacK(std::bind(&EventLoop::handleRead, this));
        m_wakeupChannel->setReadEvent();
    }

    EventLoop::~EventLoop()
    {
        m_wakeupChannel->disableAll();
        m_wakeupChannel->remove();
        ::close(m_wakeupFd);
        t_loopInThisThread = nullptr;
    }

    void EventLoop::loop()
    {
        m_isLooping = true;
        m_hasQuit = false;
        LOG_INFO("EventLoop %p start looping\n", this);
        while (!m_hasQuit)
        {
            m_activeChannels.clear();
            m_pollReturnTime = m_poller->poll(M_POLL_TIMEMS, m_activeChannels);
            for (const auto &p_channel : m_activeChannels)
            {
                p_channel->handleEvent(m_pollReturnTime);
            }
            doPendingFunctors();
        }
        LOG_INFO("EventLoop %p stop looping.\n", this);
        m_isLooping = false;
    }

    void EventLoop::quit()
    {
        m_hasQuit = true;
        if (!isInLoopThread())
        {
            wakeup();
        }
    }

    // 在当前loop中执行cb
    void EventLoop::runInLoop(std::function<void()> cb)
    {
        if (isInLoopThread())
        {
            cb();
        }
        else
        {
            queueInLoop(cb);
        }
    }

    // 把cb放入队列中 唤醒loop所在的线程执行cb
    void EventLoop::queueInLoop(std::function<void()> cb)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_pendingFunctors.emplace_back(cb);
        }

        if (!isInLoopThread() || m_callingPendingFunctors)
        {
            wakeup();
        }
    }

    void EventLoop::handleRead()
    {
        uint64_t one = 1;
        ssize_t n = read(m_wakeupFd, &one, sizeof(one));
        if (n != sizeof(one))
        {
            LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
        }
    }
    // 用来唤醒loop所在线程 向wakeupFd_写一个数据 wakeupChannel就发生读事件 当前loop线程就会被唤醒
    void EventLoop::wakeup()
    {
        uint64_t one = 1;
        ssize_t n = write(m_wakeupFd, &one, sizeof(one));
        if (n != sizeof(one))
        {
            LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
        }
    }

    // EventLoop的方法 => Poller的方法
    void EventLoop::updateChannel(Channel *channel)
    {
        m_poller->updateChannel(channel);
    }

    void EventLoop::removeChannel(Channel *channel)
    {
        m_poller->removeChannel(channel);
    }

    void EventLoop::doPendingFunctors()
    {
        std::vector<std::function<void()>> functors;
        m_callingPendingFunctors = true;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁
            // 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
            functors.swap(m_pendingFunctors);
        }

        for (const std::function<void()> &functor : functors)
        {
            functor(); // 执行当前loop需要执行的回调操作
        }

        m_callingPendingFunctors = false;
    }

} // namespace m_libnet

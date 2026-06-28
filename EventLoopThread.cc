#include "EventLoopThread.h"
#include "EventLoop.h"

namespace m_libnet
{

    EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                     const std::string &name)
        : m_loop(nullptr)
        , m_isExist(false)
        , m_thread(std::bind(&EventLoopThread::threadFunc, this), name)
        , m_mutex()
        , m_cv()
        , m_cb(cb)
    {
    }

    EventLoopThread::~EventLoopThread()
    {
        m_isExist = true;
        if (m_loop != nullptr)
        {
            m_loop->quit();
            m_thread.join();
        }
    }

    EventLoop *EventLoopThread::startLoop()
    {
        m_thread.start(); // 启用底层线程Thread类对象m_thread中通过start()创建的线程

        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]()
                      { return m_loop != nullptr; });
            loop = m_loop;
        }
        return loop;
    }

    // 下面这个方法 是在单独的新线程里运行的
    void EventLoopThread::threadFunc()
    {
        EventLoop loop; // 创建一个独立的EventLoop对象 和上面的线程是一一对应的 级one loop per thread

        if (m_cb)
        {
            m_cb(&loop);
        }

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_loop = &loop;
            m_cv.notify_one();
        }
        loop.loop(); // 执行EventLoop的loop() 开启了底层的Poller的poll()
        std::unique_lock<std::mutex> lock(m_mutex);
        m_loop = nullptr;
    }
}
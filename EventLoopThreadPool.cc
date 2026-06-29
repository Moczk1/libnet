#include <memory>

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "Logger.h"

namespace m_libnet
{

    EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
        : m_baseLoop(baseLoop), m_name(nameArg), m_istarted(false), m_numThreads(0), m_next(0)
    {
    }

        EventLoopThreadPool::~EventLoopThreadPool()
    {
        // Don't delete loop, it's stack variable
    }

    void EventLoopThreadPool::start(const ThreadInitCallback &cb)
    {
        m_istarted = true;

        for (int i = 0; i < m_numThreads; ++i)
        {
            char buf[m_name.size() + 32];
            snprintf(buf, sizeof buf, "%s%d", m_name.c_str(), i);
            EventLoopThread *t = new EventLoopThread(cb, buf);
            m_threads.push_back(std::unique_ptr<EventLoopThread>(t));
            m_loops.push_back(t->startLoop()); // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
        }

        if (m_numThreads == 0 && cb) // 整个服务端只有一个线程运行baseLoop
        {
            cb(m_baseLoop);
        }
    }

    // 如果工作在多线程中，m_baseLoop(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop *EventLoopThreadPool::getNextLoop()
    {
        // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor
        // 那么轮询只有一个线程 getNextLoop()每次都返回当前的m_baseLoop
        EventLoop *loop = m_baseLoop;

        // 通过轮询获取下一个处理事件的loop
        // 如果没设置多线程数量，则不会进去，相当于直接返回baseLoop
        if (!m_loops.empty())
        {
            loop = m_loops[m_next];
            ++m_next;
            // 轮询
            if (m_next >= m_loops.size())
            {
                m_next = 0;
            }
        }

        return loop;
    }

    std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
    {
        if (m_loops.empty())
        {
            return std::vector<EventLoop *>(1, m_baseLoop);
        }
        else
        {
            return m_loops;
        }
    }
}
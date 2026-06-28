#include <thread>
#include <condition_variable>
#include <mutex>
#include <string>

#include "Thread.h"
#include "EventLoop.h"

namespace m_libnet
{

    class EventLoop;

    class EventLoopThread
    {
    public:
        using ThreadInitCallback = std::function<void(EventLoop *)>;

        EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                        const std::string &name = std::string());
        ~EventLoopThread();

        EventLoop *startLoop();

    private:
        void threadFunc();

        EventLoop *m_loop;
        bool m_isExist;
        Thread m_thread;
        std::mutex m_mutex;             // 互斥锁
        std::condition_variable m_cv; // 条件变量
        ThreadInitCallback m_cb;
    };
}
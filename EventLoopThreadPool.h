#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace m_libnet
{

    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool
    {
        using ThreadInitCallback = std::function<void(EventLoop *)>;

    private:
        // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop
        EventLoop *m_baseLoop;

        // 线程池名称，通常由用户指定，线程池中EventLoopThread名称依赖于线程池名称。
        std::string m_name;

        // 是否已经启动标志
        bool m_istarted;

        // 线程池中线程的数量
        int m_numThreads;

        // 新连接到来，所选择EventLoop的索引
        int m_next;

        // IO线程的列表
        std::vector<std::unique_ptr<EventLoopThread>> m_threads;

        // 线程池中EventLoop的列表，指向的是EVentLoopThread线程函数创建的EventLoop对象。
        std::vector<EventLoop *> m_loops;

    public:
        EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
        ~EventLoopThreadPool();

        void setThreadNum(int numThreads) { this->m_numThreads = numThreads; }

        void start(const ThreadInitCallback &cb = ThreadInitCallback());

        // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
        EventLoop *getNextLoop();

        std::vector<EventLoop *> getAllLoops(); // 获取所有的EventLoop

        bool started() const { return m_istarted; }        // 是否已经启动
        const std::string name() const { return m_name; } // 获取名字
    };

}
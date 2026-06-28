#pragma once
#include <chrono>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include "EventLoop.h"


namespace m_libnet
{
    class Channel
    {
    public:
        enum Status
        {
            JustCreate,  // 刚刚创建，尚未赋予归属的 poller
            AddedInLoop, // 处在某个所属的 poller 中
            Secede,      // 已从某个 poller 中脱离
        };
        enum EventStatus
        {
            NONE = 0,
            READ = EPOLLIN | EPOLLPRI,
            WRITE = EPOLLOUT,
        };

        using TimePoint = std::chrono::system_clock::time_point;
        using ReadCallBackType = std::function<void(TimePoint)> ;
        using NormalCallBackType = std::function<void()>;
    private:
        const int m_fd;
        int m_events;    // EventStatus
        int m_retEvents; // EventStatus
        Status m_status;

        EventLoop *m_loop;

        std::weak_ptr<void> m_tie;
        bool m_tied;

        ReadCallBackType m_readCallBack;
        NormalCallBackType m_writeCallBack;
        NormalCallBackType m_closeCallBack;
        NormalCallBackType m_errorCallBack;

    // private:
    //     void update(); // Poller 结构中更新自己的注册事件
    public:
        Channel(EventLoop *loop, int fd);

        void setReadCallBacK(ReadCallBackType cb) { m_readCallBack = std::move(cb); }
        void setWriteCallBacK(NormalCallBackType cb) { m_writeCallBack = std::move(cb); }
        void setCloseCallBacK(NormalCallBackType cb) { m_closeCallBack = std::move(cb); }
        void setErrorCallBacK(NormalCallBackType cb) { m_errorCallBack = std::move(cb); }

        void setReadEvent() { m_events |= READ; update(); }
        void setWriteEvent() { m_events |= WRITE;update(); }
        void unsetReadEvent() { m_events |= ~READ;update(); }
        void unsetWriteEvent() { m_events |= ~WRITE;update(); }


        // 设置fd相应的事件状态 相当于epoll_ctl add delete
        void enableReading() { m_events |= READ; update(); }
        void disableReading() { m_events &= ~READ; update(); }
        void enableWriting() { m_events |= WRITE; update(); }
        void disableWriting() { m_events &= ~WRITE; update(); }
        void disableAll() { m_events = NONE; update(); }
    
        bool isNoneEvent() const { return m_events & NONE; }
        bool isWriting() const { return m_events & WRITE; }
        bool isReading() const { return m_events & READ; }

        Status getStatus() const { return m_status; }
        void setStatus(Status status) { m_status = status; }
        
        int fd() const {return m_fd;}
        int events() const {return m_events;}
        void setRetEvents(int event){ m_retEvents = event; }


        EventLoop *ownerLoop() const { return m_loop; }

        void remove() { m_loop->removeChannel(this); }
        void update() { m_loop->updateChannel(this); }

        void tie(const std::shared_ptr<void> &);
        void handleEvent(TimePoint receiveTime);
        void handleEventWithGuard(TimePoint receiveTime);
    };
} // namespace m_libnet

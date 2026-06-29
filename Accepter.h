#pragma once
#include <functional>
#include "Socket.h"
#include "Channel.h"

namespace m_libnet
{
    class Socket;
    class Channel;
    class EventLoop;
    class InetAddress;

    class Accepter
    {
        using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    private:
        NewConnectionCallback m_cb = nullptr;   // 连接建立时的具体执行过程
        EventLoop *m_loop;
        Socket m_acceptSocket;
        Channel m_acceptChannel;
        bool m_isListening = false;

        void handleRead(); // 处理新用户的连接事件

    public:
        Accepter(EventLoop *, const InetAddress &, bool);
        ~Accepter();

        void setNewConnectionCallBack(const NewConnectionCallback &cb) { m_cb = cb; }

        bool isListening() const { return m_isListening; }

        void listen();
    };
}

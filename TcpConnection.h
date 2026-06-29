#pragma once
#include <functional>
#include <chrono>
#include <memory>
#include <string>
#include <atomic>

#include "InetAddress.h"
#include "Buffer.h"

namespace m_libnet
{

    class Channel;
    class EventLoop;
    class Socket;

    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {
    private:
        /* data */
        enum State
        {
            Disconnected,
            Connecting,
            Connected,
            Disconnecting
        };

        const std::string m_name;
        std::atomic_int m_state;

        bool m_reading = false;
        EventLoop *m_loop;
        std::unique_ptr<Socket> m_socket;
        std::unique_ptr<Channel> m_channel;

        const InetAddress m_localAddr;
        const InetAddress m_peerAddr;

        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

        using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
        using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

        using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                                   Buffer *,
                                                   std::chrono::system_clock::time_point)>;
        ConnectionCallback m_connectionCallBack;
        MessageCallback m_messageCallBack;
        WriteCompleteCallback m_wrCompCallback;
        HighWaterMarkCallback m_hwCallBack;
        CloseCallback m_csCallBack;
        size_t m_hwThreshold;

        Buffer inputBuffer_;  // 接收数据的缓冲区
        Buffer outputBuffer_; // 发送数据的缓冲区 用户send向outputBuffer_发

    private:
        void handleRead(std::chrono::system_clock::time_point receiveTime);
        void handleWrite(); // 处理写事件
        void handleClose();
        void handleError();

        void sendInLoop(const void *data, size_t len);
        void shutdownInLoop();
        void sendFileInLoop(int fileDescriptor, off_t offset, size_t count);

        void setState(State sta){ m_state = sta;}

    public:
        TcpConnection(EventLoop *loop,
                      const std::string &nameArg,
                      int sockfd,
                      const InetAddress &localAddr,
                      const InetAddress &peerAddr);
        ~TcpConnection();

        EventLoop *getLoop() const { return m_loop; }
        const std::string &name() const { return m_name; }
        const InetAddress &localAddress() const { return m_localAddr; }
        const InetAddress &peerAddress() const { return m_peerAddr; }
        bool connected() const { return m_state == Connected; }

        // 发送数据
        void send(const std::string &buf);
        void sendFile(int fileDescriptor, off_t offset, size_t count);

        // 关闭半连接
        void shutdown();

        void setConnectionCallback(const ConnectionCallback &cb)
        {
            m_connectionCallBack = cb;
        }
        void setMessageCallback(const MessageCallback &cb)
        {
            m_messageCallBack = cb;
        }
        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            m_wrCompCallback = cb;
        }
        void setCloseCallback(const CloseCallback &cb)
        {
            m_csCallBack = cb;
        }
        void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
        {
            m_hwCallBack = cb;
            m_hwThreshold = highWaterMark;
        }

        // 连接建立
        void connectEstablished();
        // 连接销毁
        void connectDestroyed();
    };
} // namespace m_libnet

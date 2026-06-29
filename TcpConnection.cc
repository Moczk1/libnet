#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

namespace m_libnet
{

    static EventLoop *CheckLoopNotNull(EventLoop *loop)
    {
        if (loop == nullptr)
        {
            LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
        }
        return loop;
    } 

    TcpConnection::TcpConnection(EventLoop *loop,
    const std::string &nameArg,
    int sockfd,
    const InetAddress &localAddr,
    const InetAddress &peerAddr)
    :m_loop(CheckLoopNotNull(loop))
    , m_name(nameArg)
    , m_state(Connecting)
    , m_reading(true)
    , m_socket(new Socket(sockfd))
    , m_channel(new Channel(loop, sockfd))
    , m_localAddr(localAddr)
    , m_peerAddr(peerAddr)
    , m_hwThreshold(64 * 1024* 1024)
    {
        m_channel -> setReadCallBacK(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
        m_channel -> setWriteCallBacK(std::bind(&TcpConnection::handleWrite, this));
        m_channel -> setCloseCallBacK(std::bind(&TcpConnection::handleClose, this));
        m_channel -> setErrorCallBacK(std::bind(&TcpConnection::handleError, this));

        LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", m_name.c_str(), sockfd);
        m_socket->setKeepAlive(true);
    }

    TcpConnection::~TcpConnection()
    {
        LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", m_name.c_str(), m_channel->fd(), (int)m_state);
    }

    /**
     * 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
     **/
    void TcpConnection::sendInLoop(const void *data, size_t len)
    {
        ssize_t nwrote = 0;
        size_t remaining = len;
        bool faultError = false;

        if (m_state == Disconnected) // 之前调用过该connection的shutdown 不能再进行发送了
        {
            LOG_ERROR("disconnected, give up writing");
        }

        // 表示channel_第一次开始写数据或者缓冲区没有待发送数据
        if (!m_channel->isWriting() && outputBuffer_.readableByteSize() == 0)
        {
            nwrote = ::write(m_channel->fd(), data, len);
            if(nwrote >= 0)
            {
                remaining = len - nwrote;
                if(remaining == 0 && m_wrCompCallback)
                {
                    m_loop -> queueInLoop(std::bind(m_wrCompCallback, shared_from_this()));
                }
            }
            else
            {
                nwrote = 0;
                if(errno != EWOULDBLOCK)  // EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回 等同于EAGAIN
                {
                    LOG_ERROR("TcpConnection::sendInLoop");
                    if(errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                    {
                        faultError = true;
                    }
                }
            }
        }
        /**
         * 说明当前这一次write并没有把数据全部发送出去 剩余的数据需要保存到缓冲区当中
         * 然后给channel注册EPOLLOUT事件，Poller发现tcp的发送缓冲区有空间后会通知
         * 相应的sock->channel，调用channel对应注册的writeCallback_回调方法，
         * channel的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
         * 把发送缓冲区outputBuffer_的内容全部发送完成
         **/
        if( !faultError && remaining > 0)
        {
            size_t oldLen = outputBuffer_.readableByteSize();
            if(oldLen + remaining > m_hwThreshold && oldLen < m_hwThreshold && m_hwCallBack)
            {
                m_loop->queueInLoop(
                    std::bind(m_hwCallBack, shared_from_this(), oldLen+remaining)
                );
            }
            outputBuffer_.append((char*)data + nwrote, remaining);
            if(!m_channel->isWriting())
            {
                m_channel->enableWriting();
            }
        }
    }

    void TcpConnection::shutdown()
    {
        if (m_state == Connected)
        {
            setState(Disconnecting);
            m_loop->runInLoop(
                std::bind(&TcpConnection::shutdownInLoop, this));
        }
    }


    void TcpConnection::shutdownInLoop()
    {
        if (!m_channel->isWriting()) // 说明当前outputBuffer_的数据全部向外发送完成
        {
            m_socket->shutdownWrite();
        }
    }

    void TcpConnection::connectEstablished()
    {
        setState(Connected);
        m_channel->tie(shared_from_this());
        m_channel->enableReading(); 

        //新连接建立 执行回调
        m_connectionCallBack(shared_from_this());
    }

    // 连接销毁
    void TcpConnection::connectDestroyed()
    {
        if(m_state == Connected)
        {
            setState(Disconnected);
            m_channel->disableAll();
            m_connectionCallBack(shared_from_this());
        }
        m_channel ->remove();
    }

    // 读是相对服务器而言的 当对端客户端有数据到达 服务器端检测到EPOLLIN 就会触发该fd上的回调 handleRead取读走对端发来的数据
    void TcpConnection::handleRead(std::chrono::system_clock::time_point receiveTime)
    {
        int saveErrno = 0;
        ssize_t n = inputBuffer_.readFd(m_channel->fd(), &saveErrno);

        if(n > 0)
        {
            m_messageCallBack(shared_from_this(), &inputBuffer_, receiveTime);
        }
        else if (n == 0)
        {
            handleClose();
        }
        else 
        {
            saveErrno = errno ;// ?????
            LOG_ERROR("TcpConnection::handleRead");
            handleError();
        }
    }

    void TcpConnection::handleWrite()
    {
        if(m_channel->isWriting())
        {
            int savedErrno = 0;
            ssize_t n = outputBuffer_.writeFd(m_channel->fd(), &savedErrno);
            if(n > 0)
            {
                outputBuffer_.retrieve(n);
                if(outputBuffer_.readableByteSize() == 0)
                {
                    m_channel->disableWriting();
                    if(m_wrCompCallback) 
                    {
                        m_loop->queueInLoop(
                            std::bind(m_wrCompCallback, shared_from_this())
                        );
                    }
                    if (m_state == Disconnected)
                    {
                        shutdownInLoop();
                    }
                }
            }
            else
            {
                LOG_ERROR("TcpConnection::handleWrite");
            }
        }
    }

    void TcpConnection::handleClose()
    {
        LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", m_channel->fd(), (int)m_state);
        setState(Disconnected);
        m_channel -> disableAll();
        TcpConnectionPtr connPtr(shared_from_this());
        m_connectionCallBack(connPtr);
        m_csCallBack(connPtr);
    }


    void TcpConnection::handleError()
    {
        int optval;
        socklen_t optlen = sizeof optval;
        int err = 0;
        if(::getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        {
            err = errno;
        }
        else 
        {
            err = optval;
        }
        LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", m_name.c_str(), err);
    }

    // 新增的零拷贝发送函数
    void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t count) {
        if(connected())
        {
            if(m_loop-> isInLoopThread())
            {
                sendFileInLoop(fileDescriptor, offset, count);
            }
            else
            {
                m_loop->runInLoop(
                    std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, count)
                );
            }
        } else
        {
            LOG_ERROR("TcpConnection::sendFile - not connected");
        }
    }

    void TcpConnection::send(const std::string &buf)
{
    if (m_state == Connected)
    {
        if (m_loop->isInLoopThread()) // 这种是对于单个reactor的情况 用户调用conn->send时 loop_即为当前线程
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            m_loop->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}


    // 在事件循环中执行sendfile
    void TcpConnection::sendFileInLoop(int fileDescriptor, off_t offset, size_t count) {
        ssize_t bytesSent = 0; // 发送了多少字节数
        size_t remaining = count; // 还要多少数据要发送
        bool faultError = false; // 错误的标志位

        if(m_state == Disconnected)
        {
            LOG_ERROR("disconnected, give up writing");
            return;
        }

        // 表示Channel第一次开始写数据或者outputBuffer缓冲区中没有数据
        if(!m_channel->isWriting() && outputBuffer_.readableByteSize() == 0)
        {
            bytesSent = sendfile(m_socket->fd(), fileDescriptor, &offset, remaining);
            if(bytesSent >= 0)
            {
                remaining -= bytesSent;
                if(remaining == 0 && m_wrCompCallback)
                {
                    m_loop->queueInLoop(std::bind(m_wrCompCallback, shared_from_this()));
                }
            }
            else 
            {
                if(errno != EWOULDBLOCK)
                {
                    LOG_ERROR("TcpConnection::sendFileInLoop");
                }
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }

        if(!faultError && remaining > 0)
        {
            m_loop->queueInLoop(
                std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), 
                fileDescriptor, offset, remaining));
        }

    }


}


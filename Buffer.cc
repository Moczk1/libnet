#include "Buffer.h"

namespace m_libnet
{

    Buffer::Buffer(size_t size)
    {
        m_buffer.resize(size + PRE_SAVE_PEND);
    }

    void Buffer::retieve(size_t len)
    {
        if (len < readableByteSize())
        {
            m_readStartByteIndex += len;
        }
        else
        {
            m_readStartByteIndex += readableByteSize();
        }
    }
    std::string Buffer::retrieveAsString(size_t len)
    {
        return std::string(peek(), len);
    }
    std::string Buffer::retrieveAllAsString()
    {
        return retrieveAsString(readableByteSize());
    }
    void Buffer::makeSureSpace(size_t len)
    {
        if (len < writableByteSize())
            return;
        size_t readable = readableByteSize();
        if (len + PRE_SAVE_PEND > m_buffer.size() - readableByteSize())
        {
            m_buffer.resize(PRE_SAVE_PEND + readableByteSize() + len);
        }

        std::copy(begin() + m_readStartByteIndex,
                  begin() + m_writeStartByteIndex,
                  begin() + PRE_SAVE_PEND);
        m_readStartByteIndex = PRE_SAVE_PEND;
        m_writeStartByteIndex = m_readStartByteIndex + readable;
    }

    void Buffer::append(char *data, size_t len)
    {
        makeSureSpace(len);
        std::copy(data, data + len, begin() + m_writeStartByteIndex);
        m_writeStartByteIndex += len;
    }

    size_t Buffer::readFd(int fd, int *saveErron)
    {
        char extraBuf[64 * 1024] = {0};
        iovec vec[2];
        const size_t writable = writableByteSize();
        vec[0].iov_base = begin() + m_writeStartByteIndex;
        vec[0].iov_len = writable;

        vec[1].iov_base = extraBuf;
        vec[1].iov_len = sizeof extraBuf;

        const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1;
        const ssize_t n = ::readv(fd, vec, iovcnt);

        if (n < 0)
        {
            *saveErron = errno;
        }
        else if (n <= writable)
        {
            m_writeStartByteIndex += n;
        }
        else
        {
            m_writeStartByteIndex == m_buffer.size();
            append(extraBuf, n - writable);
        }
        return n;
    }
    size_t Buffer::writeFd(int fd, int *saveErron)
    {
        ssize_t n = ::write(fd, begin(), writableByteSize());
        if (n < 0)
        {
            *saveErron = errno;
        }
        return n;
    }
}

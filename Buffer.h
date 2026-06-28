#pragma once
#include <string>
#include <vector>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

namespace m_libnet
{
    class Buffer
    {
        static const int PRE_SAVE_PEND = 8;

    private:
        /* data */
        std::vector<char> m_buffer;
        int m_readStartByteIndex = PRE_SAVE_PEND;
        int m_writeStartByteIndex = PRE_SAVE_PEND;

    public:
        explicit Buffer(size_t size);

        size_t readableByteSize() const { return m_writeStartByteIndex - m_readStartByteIndex; }
        size_t writableByteSize() const { return m_buffer.size() - m_writeStartByteIndex; }
        size_t prependableBytes() const { return m_readStartByteIndex; }

        const char *peek() const { return &*m_buffer.begin() + m_readStartByteIndex; }

        char *begin() { return &*m_buffer.begin(); }
        const char *begin() const { return &*m_buffer.begin(); }

        void retieve(size_t len);
        std::string retrieveAsString(size_t len);
        std::string retrieveAllAsString();

        void makeSureSpace(size_t len);

        void append(char *, size_t len);

        size_t readFd(int fd, int *saveErron);
        size_t writeFd(int fd, int *saveErron);
    };

    
}
#include <sys/sendfile.h>

#include "llt_muduo/net/TcpConnection.h"
#include "llt_muduo/net/Channel.h"
#include "llt_muduo/net/EventLoop.h"
#include "llt_muduo/net/Socket.h"
#include "llt_muduo/base/Logger.h"

namespace llt_muduo
{
    namespace net
    {
        static EventLoop *checkLoopNotNull(EventLoop *loop)
        {
            if (loop == nullptr)
            {
                LOG_FATAL("TcpConnection::checkLoopNotNull - EventLoop should not be null!");
            }
            return loop;
        }

        TcpConnection::TcpConnection(EventLoop *loop,
                                     const std::string &nameArg,
                                     int sockfd,
                                     const InetAddress &localAddr,
                                     const InetAddress &peerAddr)
            : loop_(checkLoopNotNull(loop)),
              name_(nameArg),
              state_(kConnecting),
              reading_(true),
              socket_(new Socket(sockfd)),
              channel_(new Channel(loop, sockfd)),
              localAddr_(localAddr),
              peerAddr_(peerAddr),
              highWaterMark_(64 * 1024 * 1024) // 64M
        {
            channel_->setReadCallback(
                std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
            channel_->setWriteCallback(
                std::bind(&TcpConnection::handleWrite, this));
            channel_->setCloseCallback(
                std::bind(&TcpConnection::handleClose, this));
            channel_->setErrorCallback(
                std::bind(&TcpConnection::handleError, this));

            LOG_INFO(std::string("TcpConnection::ctor - fd = ") + std::to_string(sockfd));
            socket_->setKeepAlive(true);
        }

        TcpConnection::~TcpConnection()
        {
            LOG_INFO(std::string("TcpConnection::dtor - fd = ") + std::to_string(channel_->fd()));
        }

        void TcpConnection::send(const std::string &message)
        {
            if (state_ == kConnected)
            {
                if (loop_->isInLoopThread())
                {
                    sendInLoop(message.c_str(), message.size());
                }
                else
                {
                    loop_->runInLoop(
                        std::bind(&TcpConnection::sendInLoop, shared_from_this(), message.c_str(), message.size()));
                }
            }
        }

        void TcpConnection::sendInLoop(const void *data, size_t len)
        {
            size_t written = 0;
            size_t remaining = len;
            bool faultError = false;

            if (state_ == kDisconnected)
            {
                LOG_ERROR("disconnected, give up writing");
            }
            if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
            {
                written = ::write(channel_->fd(), data, len);
                if (written >= 0)
                {
                    remaining = len - written;
                    if (remaining == 0 && writeCompleteCallback_)
                    {
                        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                }
                else
                {
                    written = 0;
                    if (errno != EWOULDBLOCK)
                    {
                        LOG_ERROR("TcpConnection::sendInLoop - write error");
                        if (errno == EPIPE || errno == ECONNRESET)
                        {
                            faultError = true;
                        }
                    }
                }
            }
            if (!faultError && remaining > 0)
            {
                size_t oldLen = outputBuffer_.readableBytes();
                if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_)
                {
                    loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
                }
                outputBuffer_.append(static_cast<const char *>(data) + written, remaining);
                if (!channel_->isWriting())
                {
                    channel_->enableWriting();
                }
            }
        }

        void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t size)
        {
            if (connected())
            {
                if (loop_->isInLoopThread())
                {
                    sendFileInLoop(fileDescriptor, offset, size);
                }
                else
                {
                    // runinloop因为写是重要动作
                    loop_->runInLoop(
                        std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, size));
                }
            }
            else
            {
                LOG_ERROR("disconnected, give up sending file");
            }
        }

        void TcpConnection::sendFileInLoop(int fileDescriptor, off_t offset, size_t size)
        {
            ssize_t bytesSent = 0;
            size_t remaining = size;
            bool faultError = false;

            // file大而复杂，所以建议直接关
            // send因为需要优雅的关闭，发送完缓冲区的剩余数据。
            if (state_ == kConnecting)
            {
                LOG_ERROR("connection closing, give up writing");
                return;
            }

            if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
            {
                bytesSent = sendfile(channel_->fd(), fileDescriptor, &offset, size);
                if (bytesSent >= 0)
                {
                    remaining -= bytesSent;
                    if (remaining == 0 && writeCompleteCallback_)
                    {
                        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                    else
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            LOG_ERROR("TcpConnection::sendInLoop - write error");
                        }
                        if(errno == EPIPE || errno == ECONNRESET)
                        {
                            faultError = true;
                        }
                    }   
                }
            }
            if (!faultError && remaining > 0)
            {
                loop_->queueInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(),fileDescriptor, offset, remaining));
            }
        }

        void TcpConnection::shutdown()
        {
            if (state_ == kConnected)
            {
                setState(kDisconnecting);
                // if (!channel_->isWriting())
                // {
                //     socket_->shutdownWrite();
                // }
                ///不直接关闭，因为shutdown的逻辑可能不由本身线程负责
                loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
            }
        }

        void TcpConnection::shutdownInLoop()
        {
            //优雅的关闭，即使这里失败了，之后handwrite也会检查，如果是disconnecting,会再次调用
            if (!channel_->isWriting())
            {
                socket_->shutdownWrite();
            } 
        }

        void TcpConnection::connectEstablished()
        {
            setState(kConnected);
            channel_->tie(shared_from_this());
            channel_->enableReading();

            connectionCallback_(shared_from_this());
        }    

        void TcpConnection::connectDestroyed()
        {
            //if防止服务器主动关闭连接的情况，此时handclose还未发生
            if (state_ == kConnected)
            {
                setState(kDisconnected);
                channel_->disableAll();

                connectionCallback_(shared_from_this());
            }
            channel_->remove();
        }  

        void TcpConnection::handleRead(llt_muduo::base::Timestamp receiveTime)
        {
            
            int savedErrno = 0;
            ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
            if(n > 0)
            {
                messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
            }
            else if(n==0){
                handleClose();
            }
            else{
                errno=savedErrno;
                LOG_ERROR("TcpConnection::handleRead");
                handleError();
            }
        }  

        void TcpConnection::handleWrite()
        {
            if (channel_->isWriting())
            {
                int savedErrno = 0;
                ssize_t n = outputBuffer_.writeFd(channel_->fd(),&savedErrno);
                if (n > 0)
                {
                    outputBuffer_.retrieve(n);
                    if (outputBuffer_.readableBytes() == 0)
                    {
                        channel_->disableWriting();
                        if (writeCompleteCallback_){
                            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                        }
                        if(state_ == kDisconnecting){
                            shutdownInLoop();
                        }
                    }
                }
                else{
                    //只是一个连接出现问题，可能有很多种可能，不至于fatal
                    LOG_ERROR("TcpConnection::handleWrite");
                    if(savedErrno != EWOULDBLOCK&&  savedErrno != EAGAIN){
                        handleError();
                    }

                }
            }else{
                LOG_ERROR(std::string("TcpConnection fd=%d is down, no more writing")+std::to_string(channel_->fd()));
            }
        }

        void TcpConnection::handleClose()
        {
            LOG_INFO(std::string("TcpConnection fd=")+std::to_string(channel_->fd())+" is closed");
            setState(kDisconnected);
            channel_->disableAll();

            TcpConnectionPtr guardThis(shared_from_this());
            connectionCallback_(guardThis);
            closeCallback_(guardThis);
        }

        void TcpConnection::handleError()
        {
            int optval;
            socklen_t optlen = sizeof optval;
            int err=0;
            if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
            {
                err = errno;
            }
            else {
                err = optval;
            }
            LOG_ERROR(std::string("TcpConnection::handleError name=")+name_+" - SO_ERROR="+std::to_string(err));
            handleClose();
        }
    }

}
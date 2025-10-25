#include <sys/sendfile.h>

#include "MiniMuduo/net/TcpConnection.h"
#include "MiniMuduo/net/Channel.h"
#include "MiniMuduo/net/EventLoop.h"
#include "MiniMuduo/net/Socket.h"
#include "MiniMuduo/base/Logger.h"

namespace MiniMuduo
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
                                     const InetAddress &peerAddr,
                                     const double idleTimeout)
            : loop_(checkLoopNotNull(loop)),
              name_(nameArg),
              state_(kConnecting),
              reading_(true),
              socket_(new Socket(sockfd)),
              channel_(new Channel(loop, sockfd)),
              localAddr_(localAddr),
              peerAddr_(peerAddr),
              highWaterMark_(64 * 1024 * 1024), // 64M
              idleTimeout_(idleTimeout)
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
            LOG_INFO("TcpConnection::send called, msg size: " + std::to_string(message.size())); // 确认被调用
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

        void TcpConnection::send(MiniMuduo::net::Buffer &&buf)
        {
            if (state_ == kConnected)
            {
                if (loop_->isInLoopThread())
                {
                    outputBuffer_.append(std::move(buf));
                    channel_->enableWriting();
                }
                else
                {
                    loop_->runInLoop([this,newbuf=std::move(buf)](){
                        outputBuffer_.append(std::move(newbuf));
                        channel_->enableWriting();
                    });
                }
            }
        }

        void TcpConnection::sendInLoop(const void *data, size_t len)
        {
            LOG_INFO("TcpConnection::sendInLoop called, msg size: " + std::to_string(len));
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
                    if(remaining == 0)
                    {
                        LOG_INFO("TcpConnection::sendInLoop - write complete in ::write");
                    }
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
                    LOG_INFO("TcpConnection::sendInLoop - enable writing remaining: " + std::to_string(remaining));
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
                //offset会自动更新
                //唯一的问题可能是errno,并且errno==EWOULDBLOCK,再次发送的时候用旧的offset,但是其实offset已经改变，已经发送了一些数据
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
            if(idleTimeout_ > 0)
            {
                resetIdleTimer(MiniMuduo::base::Timestamp::now());
            }
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

        void TcpConnection::handleRead(MiniMuduo::base::Timestamp receiveTime)
        {
            
            int savedErrno = 0;
            ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
            if(n > 0)
            {
                if(idleTimeout_ > 0)
                {
                    resetIdleTimer(receiveTime);
                }
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
            LOG_INFO("TcpConnection::handleWrite");
            if (channel_->isWriting())
            {
                int savedErrno = 0;
                ssize_t n = outputBuffer_.writeFd(channel_->fd(),&savedErrno);
                if (n > 0)
                {
                    if(idleTimeout_ > 0)
                    {
                        resetIdleTimer(MiniMuduo::base::Timestamp::now());
                    }
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
                    //EAGAIN表示资源当前不可用，即当前缓冲区没有数据，在非阻塞情况下很有可能出现这个问题
                    //EWOULDBLOCK操作会阻塞（非阻塞模式下不可立即完成）
                    //两者的处理方式都是稍后重试或等待
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
            if(idleTimerId_)
            {
                loop_->cancelTimer(*idleTimerId_);
                idleTimerId_.reset();
            }
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

        void TcpConnection::resetIdleTimer(MiniMuduo::base::Timestamp now) {
            loop_->assertInLoopThread();
            
            // 1. 如果之前已经有定时器，先取消它
            if (idleTimerId_) {
                loop_->cancelTimer(*idleTimerId_);
            }

            // 2. 创建一个新的超时回调
            //    【关键】使用 weak_ptr 防止循环引用，并安全地检查对象存活
            std::weak_ptr<TcpConnection> weakSelf = shared_from_this();
            auto idleCallback = [weakSelf]() {
                TcpConnectionPtr self = weakSelf.lock();
                if (self) {
                    // 如果连接还存活，就执行关闭操作
                    LOG_INFO("Connection " +self->name() + " timed out, shutting down.");
                    self->shutdown(); 
                }
            };

            // 3. 添加一个新的定时器，并保存返回的 TimerId
            if(idleTimeout_ > 0)
            idleTimerId_ = loop_->runAt(now+idleTimeout_, std::move(idleCallback));
        }
        void TcpConnection::setIdleTimeout(double seconds)
        {
            idleTimeout_ = seconds;
        }
        void TcpConnection::cancelIdleTimeout()
        {
            loop_->runInLoop([this]() {
                loop_->assertInLoopThread();
                setIdleTimeout(0.0);
                resetIdleTimer(MiniMuduo::base::Timestamp::now());
                idleTimerId_.reset();
            });
        }
    }

}
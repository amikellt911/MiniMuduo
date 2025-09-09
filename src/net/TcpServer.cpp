#include<string.h>

#include "MiniMuduo/net/TcpServer.h"
#include "MiniMuduo/base/Logger.h"
#include "MiniMuduo/net/EventLoop.h"
#include "MiniMuduo/net/EventLoopThreadPool.h"
#include "MiniMuduo/net/Acceptor.h"
#include "MiniMuduo/net/InetAddress.h"
#include "MiniMuduo/net/TcpConnection.h"

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

        TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
            : loop_(checkLoopNotNull(loop)),
              ipPort_(listenAddr.toIpPort()),
              name_(nameArg), // 是==，不是=，不是缺省
              acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
              threadPool_(new EventLoopThreadPool(loop, name_)),
              connectionCallback_(),
              messageCallback_(),
              nextConnId_(1),
              started_(0)
        {
            acceptor_->setNewConnectionCallback(
                std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
        }
        TcpServer::~TcpServer()
        {
            for (auto &item : connections_)
            {
                TcpConnectionPtr conn(item.second);
                item.second.reset(); // 原始的map中的智能指针复位，现在就剩栈中的了
                conn->getLoop()->runInLoop(
                    std::bind(&TcpConnection::connectDestroyed, conn));
            }
        }

        void TcpServer::setThreadNum(int nums)
        {
            numThreads_ = nums;
            threadPool_->setThreadNum(numThreads_);
        }

        void TcpServer::start()
        {
            if (started_.fetch_add(1) == 0)
            { // 防止多次调用
                threadPool_->start(threadInitCallback_,cancelThreshold_);
                loop_->runInLoop(
                    std::bind(&Acceptor::listen, acceptor_.get()));
            }
        }

        void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
        {
            EventLoop *ioLoop = threadPool_->getNextLoop();
            char buf[64] = {0};
            snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
            // 只在mainloop进行，不需要设置为atomic_int
            ++nextConnId_;
            std::string connName = name_ + buf;
            LOG_INFO(std::string("TcpServer::newConnection - ") + connName + " from " + peerAddr.toIpPort());
            sockaddr_in local;
            ::memset(&local, 0, sizeof local);
            socklen_t addrlen = sizeof local;
            if (::getsockname(sockfd, reinterpret_cast<sockaddr *>(&local), &addrlen) < 0)
            {
                LOG_ERROR("TcpServer::newConnection - getsockAddr");
            }
            InetAddress localAddr(local);
            TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr,idleTimeout_));
            connections_[connName] = conn;
            conn->setConnectionCallback(connectionCallback_);
            conn->setMessageCallback(messageCallback_);
            conn->setWriteCompleteCallback(writeCompleteCallback_);
            conn->setCloseCallback(
                std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
            ioLoop->runInLoop(
                std::bind(&TcpConnection::connectEstablished, conn));
        }

        void TcpServer::removeConnection(const TcpConnectionPtr &conn)
        {
            loop_->runInLoop(
                std::bind(&TcpServer::removeConnectionInLoop, this, conn));
        }

        void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
        {
            LOG_INFO(std::string("TcpServer::removeConnectionInLoop - ") + conn->name());
            connections_.erase(conn->name());
            EventLoop *ioLoop = conn->getLoop();
            //runinloop因为在队列中可能立刻执行，但是此时loop可能正在遍历activeChannel
            //这时删除channel可能会有问题
            ioLoop->queueInLoop(
                std::bind(&TcpConnection::connectDestroyed, conn));
        }
    }
}
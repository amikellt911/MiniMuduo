#pragma once

#include <memory>
#include <atomic>
#include <optional> // C++17, 如果没有可以用 pair<bool, int64_t>

#include "MiniMuduo/base/noncopyable.h"
#include "MiniMuduo/base/Timestamp.h"
#include "MiniMuduo/net/Buffer.h"
#include "MiniMuduo/net/InetAddress.h"
#include "MiniMuduo/net/Callbacks.h"

namespace MiniMuduo
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Socket;

        class TcpConnection : MiniMuduo::base::noncopyable,
                            public std::enable_shared_from_this<TcpConnection>
        {
            public:
                TcpConnection(EventLoop *loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr,
                const double idleTimeout
                );
                ~TcpConnection();

                EventLoop *getLoop() const { return loop_; }
                const std::string &name() const {return name_;}
                const InetAddress &localAddress() const {return localAddr_;}
                const InetAddress &peerAddress() const {return peerAddr_;}
                bool connected() const { return state_ == kConnected; }
                
                void send(const std::string &message);
                void sendFile(int fileDescriptor,off_t offset,size_t size);
                //关闭写端
                void shutdown();

                void setConnectionCallback(const ConnectionCallback &cb)
                { connectionCallback_ = std::move(cb); }
                void setMessageCallback(const MessageCallback &cb)
                { messageCallback_ = std::move(cb); }
                void setWriteCompleteCallback(const WriteCompleteCallback &cb){
                    writeCompleteCallback_ = std::move(cb);
                }
                void setCloseCallback(const CloseCallback &cb){
                    closeCallback_ = std::move(cb);
                }
                void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t highWaterMark)
                {
                    highWaterMarkCallback_ = std::move(cb);
                    highWaterMark_ = highWaterMark;
                }
                void connectEstablished();
                void connectDestroyed();
                void setIdleTimeout(double seconds);
            private:
                enum StateE
                {
                    kConnecting,
                    kConnected,
                    kDisconnecting,
                    kDisconnected
                };
                void setState(StateE s) { state_ = s; }

                void handleRead(MiniMuduo::base::Timestamp receiveTime);
                void handleWrite();
                void handleClose();
                void handleError();

                void sendInLoop(const void *data, size_t len);
                void shutdownInLoop();
                void sendFileInLoop(int fileDescriptor,off_t offset,size_t size);

                // 封装了定时器创建/重置逻辑的私有方法
                void resetIdleTimer(MiniMuduo::base::Timestamp now);

            private:
                EventLoop *loop_;
                const std::string name_;
                const InetAddress localAddr_;
                const InetAddress peerAddr_;
                std::atomic_bool state_;
                bool reading_;

                std::unique_ptr<Socket> socket_;
                std::unique_ptr<Channel> channel_;

                ConnectionCallback connectionCallback_;
                MessageCallback messageCallback_;
                WriteCompleteCallback writeCompleteCallback_;
                HighWaterMarkCallback highWaterMarkCallback_;
                CloseCallback closeCallback_;
                size_t highWaterMark_;

                Buffer inputBuffer_;
                Buffer outputBuffer_;

                // 新增的超时管理成员
                std::optional<int64_t> idleTimerId_; // 用于保存和取消定时器
                const double idleTimeout_;
        };
    } 
}
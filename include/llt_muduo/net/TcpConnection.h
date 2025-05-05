#pragma once

#include <memory>
#include <atomic>

#include "llt_muduo/base/noncopyable.h"
#include "llt_muduo/base/Timestamp.h"
#include "llt_muduo/net/Buffer.h"
#include "llt_muduo/net/InetAddress.h"
#include "llt_muduo/net/Callbacks.h"

namespace llt_muduo
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Socket;

        class TcpConnection : llt_muduo::base::noncopyable,
                            public std::enable_shared_from_this<TcpConnection>
        {
            public:
                TcpConnection(EventLoop *loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr
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
            private:
                enum StateE
                {
                    kConnecting,
                    kConnected,
                    kDisconnecting,
                    kDisconnected
                };
                void setState(StateE s) { state_ = s; }

                void handleRead(llt_muduo::base::Timestamp receiveTime);
                void handleWrite();
                void handleClose();
                void handleError();

                void sendInLoop(const void *data, size_t len);
                void shutdownInLoop();
                void sendFileInLoop(int fileDescriptor,off_t offset,size_t size);

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
        };
    } 
}
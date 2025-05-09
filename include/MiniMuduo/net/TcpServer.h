#pragma once

#include<memory>
#include<unordered_map>
#include<functional>
#include<atomic>
#include"MiniMuduo/net/Callbacks.h"
#include"MiniMuduo/base/noncopyable.h"


namespace MiniMuduo
{
    namespace net
    {
        class EventLoop;
        class Acceptor;
        class EventLoopThreadPool;
        class InetAddress;
        class TcpConnection;
        class TcpServer : MiniMuduo::base::noncopyable
        { 

            public:
                using ThreadInitCallback = std::function<void(EventLoop*)>;
                enum Option
                {
                    kNoReusePort,
                    kReusePort
                };
            public:
            //缺省只声明一次，通常在.h
                TcpServer(EventLoop *loop,const InetAddress &listenAddr,const std::string &nameArg,Option option = kNoReusePort);
                ~TcpServer();
                void setThreadNum(int nums);
                void start();
                void setConnectionCallback(const ConnectionCallback &cb)
                {
                    connectionCallback_ = std::move(cb);
                }
                void setThreadInitCallback(const ThreadInitCallback &cb)
                {
                    threadInitCallback_ = std::move(cb);
                }
                void setMessageCallback(const MessageCallback &cb)
                {
                    messageCallback_ = std::move(cb);
                }
                void setWriteCompleteCallback(const WriteCompleteCallback &cb)
                {
                    writeCompleteCallback_ = std::move(cb);
                }

            
            private:
                void newConnection(int sockfd,const InetAddress &peerAddr);
                void removeConnection(const TcpConnectionPtr &conn);
                void removeConnectionInLoop(const TcpConnectionPtr &conn);

            private:
                using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;
                EventLoop *loop_;

                const std::string name_;
                const std::string ipPort_;

                std::unique_ptr<Acceptor> acceptor_;
                //share因为tcpconnection是shared，为了生命周期的管理
                //因为他被多个对象所依赖，包括tcpserver,connection的eventloop
                std::shared_ptr<EventLoopThreadPool> threadPool_;

                ConnectionCallback connectionCallback_;
                MessageCallback messageCallback_;
                WriteCompleteCallback writeCompleteCallback_;
                ThreadInitCallback threadInitCallback_;

                int numThreads_;
                std::atomic_int started_;
                int nextConnId_;
                ConnectionMap connections_;
        };
    }
}
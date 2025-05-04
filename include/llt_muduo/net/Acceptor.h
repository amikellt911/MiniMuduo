#pragma once

//如果是拥有关系，需要知道该类型的完整定义
//要创建该实例作为成员变量，或者继承该类
//头文件的一些内联函数需要调用一些成员变量或者函数
//都需要完整的头文件
#include "llt_muduo/base/noncopyable.h"
#include "llt_muduo/net/Socket.h"
#include "llt_muduo/net/Channel.h"

namespace llt_muduo
{
    namespace net
    {
        //前向声明
        //只需要指针或引用，我不需要知道他的内部的结构，构造函数
        //更奇妙的地方，
        //.cpp没有EventLoop.h
        //因为.cpp只需要通过loop初始化Channel
        //.cpp没有使用loop的成员函数和成员变量的地方
        //所以东西都通过Channel来调用loop来操作
        //所以Channel.cpp里面有EventLoop.h，
        //这叫做最小头文件包含，太妙了
        class EventLoop;
        class InetAddress;
        //根本是指针/引用 vs 对象/继承
        //但是最佳实践还是跟加速编译有关，减少编译的依赖
        //比如EventLoop修改后，包含Acceptor.h的文件不需要重新编译（如果不包含EventLoop.h的话）
    
        class Acceptor : llt_muduo::base::noncopyable {
            public:
                using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

                Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
                ~Acceptor();

                void setNewConnectionCallback(const NewConnectionCallback &cb)
                {
                    newConnectionCallback_ = cb;
                }
                bool listening() const {return listening_;};

                void listen();

            private:
                void handleRead();

                EventLoop *loop_;
                Socket acceptSocket_;
                Channel acceptChannel_;
                NewConnectionCallback newConnectionCallback_;
                bool listening_;
        };
    } 
}
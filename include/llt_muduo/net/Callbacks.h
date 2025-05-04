#pragma once 

#include "memory"
#include"functional"
#include "llt_muduo/base/Timestamp.h"
namespace llt_muduo{
    namespace net{
        class Buffer;
        class TcpConnection;
        class llt_muduo::base::Timestamp;

        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, llt_muduo::base::Timestamp)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
        using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
    }
}
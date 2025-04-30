#pragma once 

#include "memory"
#include"functional"

namespace llt_muduo{
    namespace net{
        class Buffer;
        class TcpConnection;
        class base::Timestamp;

        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, base::Timestamp)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
        using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
    }
}
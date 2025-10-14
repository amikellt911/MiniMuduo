#include"MiniMuduo/net/TcpServer.h"
#include"MiniMuduo/base/Logger.h"
#include"MiniMuduo/net/TcpConnection.h"
#include"MiniMuduo/net/EventLoop.h"
class EchoServer{
    public:
        EchoServer(MiniMuduo::net::EventLoop *loop,const  MiniMuduo::net::InetAddress &listenAddr,const std::string &nameArg)
            :  server_(loop,listenAddr,nameArg)
            ,loop_(loop)
            {
                server_.setConnectionCallback(
                    std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
                    server_.setMessageCallback(
                    std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
                    server_.setThreadNum(4);
            }

        void start()
        {
            server_.start();
        }
        void setIdleTimeout(double seconds)
        {
            server_.setIdleTimeout(seconds);
        }
        void setCancelThreshold(double seconds)
        {
            server_.setCancelThreshold(seconds);
        }
    private:
        void onConnection(const MiniMuduo::net::TcpConnectionPtr &conn)
        {
            if(conn->connected())
            {
                LOG_INFO(std::string("Connection Up") + conn->peerAddress().toIpPort());
            }
            else 
            {
                LOG_INFO(std::string("Connection Down") + conn->peerAddress().toIpPort());
            }
        }
        void onMessage(const MiniMuduo::net::TcpConnectionPtr &conn,MiniMuduo::net::Buffer *buffer,MiniMuduo::base::Timestamp receiveTime)
        {
            // // 为了能用 ab 测试，我们简单地回复一个 HTTP OK
            // // 1. 先接收并清空客户端发来的数据
            // std::string request = buffer->retrieveAllAsString();
            
            // // 2. 构造一个最小的 HTTP 响应
            // std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nHi\n";
            // LOG_ERROR(request);
            // // 3. 发送回去
            // conn->send(response);
            // LOG_ERROR("OnMessage Finished");
            // conn->shutdown();
            // LOG_INFO("Shutdown initiated.");
            // 使用一个循环来处理所有可能在一个批次中到达的请求
            while (true) 
            {
                // 1. 使用我们新的“侦察兵”来查找一个完整的 HTTP 请求的边界。
                //    我们这里只简单地处理没有 body 的 GET 请求，所以边界就是请求头末尾的 "\r\n\r\n"。
                const char* end_of_request = buffer->findCRLFCRLF();

                // 2. 如果找不到边界 (返回 nullptr)，说明 Buffer 里的数据还不够构成一个完整的请求。
                if (end_of_request == nullptr) {
                    // 我们什么都不做，直接退出 onMessage。
                    // 剩下的数据会继续留在 Buffer 里，等待下一次 handleRead 带来更多数据。
                    break; 
                }

                // 3. 如果找到了边界，我们就知道一个完整的请求有多长。
                //    我们创建一个 HttpRequest 对象来存放它（这里为了简单，直接用 string）。
                //    注意：peek() 指向可读数据的开头，end_of_request 指向 "\r\n\r\n" 的开头。
                //    所以请求的总长度是 (end_of_request - buffer->peek() + 4)。
                size_t request_len = (end_of_request - buffer->peek()) + 4;
                std::string request_str(buffer->peek(), request_len);
                
                // 【重要】从 Buffer 中取走我们刚刚处理完的这个请求。
                buffer->retrieve(request_len);

                LOG_INFO ("Received one complete request:\n" +request_str);

                // 4. 构造并发送响应。
                //    对于长连接，HTTP/1.1 协议要求响应头里包含 Connection: Keep-Alive
                //    以及一个正确的 Content-Length。ab 工具能识别这些。
                std::string body = "Hi";
                std::string response = "HTTP/1.1 200 OK\r\n";
                response += "Connection: Keep-Alive\r\n";
                response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
                response += "\r\n";
                response += body;
                
                conn->send(response);
                
                // 5. 循环会继续，检查 Buffer 中是否还有剩余的数据，
                //    这些数据可能是下一个完整的请求，或者下一个请求的一部分。
            }
        }
    private:
        MiniMuduo::net::TcpServer server_;
        MiniMuduo::net::EventLoop *loop_;
};

int main(){
    MiniMuduo::base::Logger::instance().setGlobalLogLevel(MiniMuduo::base::LogLevel::FATAL);
    MiniMuduo::net::EventLoop loop;
    MiniMuduo::net::InetAddress addr(8080);
    EchoServer server(&loop,addr,"TestServer");
    //因为start里面设置了cancelThreshold，所以必须要在这之前搞
    //先设置统一时间，然后在回调用户信息的时候，自动调整
    //server.setIdleTimeout(60.0);
    server.setCancelThreshold(10.0);
    server.start();
    loop.loop();
    return 0;
}
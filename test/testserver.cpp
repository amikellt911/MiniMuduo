#include"llt_muduo/net/TcpServer.h"
#include"llt_muduo/base/Logger.h"
#include"llt_muduo/net/TcpConnection.h"
#include"llt_muduo/net/EventLoop.h"
class EchoServer{
    public:
        EchoServer(llt_muduo::net::EventLoop *loop,const  llt_muduo::net::InetAddress &listenAddr,const std::string &nameArg)
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
    private:
        void onConnection(const llt_muduo::net::TcpConnectionPtr &conn)
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
        void onMessage(const llt_muduo::net::TcpConnectionPtr &conn,llt_muduo::net::Buffer *buffer,llt_muduo::base::Timestamp receiveTime)
        {
            std::string msg(buffer->retrieveAllAsString());
            conn->send(msg);
        }
    private:
        llt_muduo::net::TcpServer server_;
        llt_muduo::net::EventLoop *loop_;
};

int main(){
    llt_muduo::net::EventLoop loop;
    llt_muduo::net::InetAddress addr(8080);
    EchoServer server(&loop,addr,"TestServer");
    server.start();
    loop.loop();
    return 0;
}
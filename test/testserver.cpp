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
            std::string msg(buffer->retrieveAllAsString());
            conn->send(msg);
        }
    private:
        MiniMuduo::net::TcpServer server_;
        MiniMuduo::net::EventLoop *loop_;
};

int main(){
    MiniMuduo::net::EventLoop loop;
    MiniMuduo::net::InetAddress addr(8080);
    EchoServer server(&loop,addr,"TestServer");
    server.start();
    loop.loop();
    return 0;
}
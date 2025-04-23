#pragma once

#include<functional>//仿函数
#include<memory>//智能指针

#include "llt_muduo/base/Timestamp.h" "
#include "llt_muduo/base/noncopyable.h"

namespace llt_muduo
{
    namespace net
    {

        class EventLoop;//前向声明

        class Channel:base::noncopyable{
            public:
                using EventCallback=std::function<void()>;
                using ReadEventCallback = std::function<void(base::Timestamp)>;

                Channel(EventLoop* loop,int sockfd);

                ~Channel();

                void handleEvent(base::Timestamp receiveTime);

                void setReadCallback(ReadEventCallback cb){readCallback_=std::move(cb);}

                void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}

                void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}

                void setErrorCallback(EventCallback cb){errorCallback_=std::move(cb);}
                //获取tcpconnection的weak_ptr,防止conn释放，channel被手动remove的，channel还在执行回调
                void tie(const std::shared_ptr<void>&);
                
                int fd() const {return fd_;}
                int events() const {return events_;}
                void set_revents(int revt){revents_=revt;}
               
               void enableReading(){
                events_|=kReadEvent;
                update();
               }

               void disableReading(){
                events_&=~kReadEvent;
                update();
               }

               void enableWriting(){
                events_|=kWriteEvent;
                update();
               }

               void disableWriting(){
                events_&=~kWriteEvent;
                update();
               }

               void disableAll(){
                events_=kNoneEvent;
                update();
               }
               
               bool isNoneEvent() const {return events_==kNoneEvent;}

               bool isWriting() const {return events_&kWriteEvent;}

               bool isReading() const {return events_&kReadEvent;}

               int index(){return index_;}

               void set_index(int idx){index_=idx;}

               EventLoop* ownerLoop(){return loop_;}

               void remove();
               
            private:
                ReadEventCallback readCallback_;
                EventCallback writeCallback_;
                EventCallback closeCallback_;
                EventCallback errorCallback_;

                EventLoop* loop_;

                void update();

                void handleEventWithGuard(base::Timestamp receiveTime);

                static const int kNoneEvent;
                
                static const int kReadEvent;

                static const int kWriteEvent;

                const int fd_;

                int events_;

                int revents_;

                int index_;

                std::weak_ptr<void> tie_;

                bool tied_;
                
        };

    }
}
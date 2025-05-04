#pragma once

#include<functional>
#include<memory>
#include<thread>
#include<condition_variable>
#include<string>
#include<atomic>
#include "llt_muduo/base/noncopyable.h"

namespace llt_muduo
{
    namespace net
    {
        class Thread:llt_muduo::base::noncopyable{
            public:
                using ThreadFunc=std::function<void()>;
                //防止隐式类型转换，增加代码可读性，是良好的编程实践
                //因为Thread比较重要，副作用也比较明显
                explicit Thread(ThreadFunc func,const std::string &name=std::string());
                ~Thread();

                void start();
                void join();

                bool started()const{return started_;}

                pid_t tid()const{return tid_;}

                const std::string& name()const{return name_;}

                static int numCreated(){return numCreated_;}

            private:
                void setDefaultName();

                bool started_;
                bool joined_;

                //为了raii,不用自己join，但是也许unique_ptr会更好？
                std::shared_ptr<std::thread> thread_;

                pid_t tid_;
                ThreadFunc func_;
                std::string name_;
                //static是类变量，唯一
                //显示已经创建了几个线程
                //负责唯一的序号或者命名
                //atomic_int，是因为害怕如果同时创建几个线程，会出现冲突
                static std::atomic_int numCreated_;

                std::mutex mutex_;
                std::condition_variable cond_;
        };
    }
}
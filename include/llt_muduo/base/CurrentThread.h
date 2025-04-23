#pragma once
#include <sys/syscall.h>
#include <unistd.h>
namespace llt_muduo
{
    namespace base
    {
        namespace CurrentThread
        {
            //_thread和thread_local的不同点，thread_local需要类的构造和析构的时候使用
            //_thread是编译器内置的，性能更高，兼容性好
            // 这是命名空间，没有类,因为更加方便，并且没有构造和析构的开销。
            // 所以使用extern
            // 使用_thread，没有线程创建一个自己的副本
            // 之所以要保留tid缓存，因为系统调用非常耗时，拿到tid后将其保存
            extern __thread int t_cachedTid;
            void cacheTid();
            inline int tid()
            {
                if (__builtin_expect(t_cachedTid == 0, 0))
                {
                    //__builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
                    cacheTid();
                }
                return t_cachedTid;
            }
        }
    }

}
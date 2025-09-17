# MiniMuduo

  

基于 C++11 重构 Muduo 网络库

## 项目介绍

  

本项目是在学习并参考 muduo 源码的基础上, 使用 C++11 对 muduo 网络库进行了重构, 去除 muduo 对 boost 库的依赖，并根据 muduo 的设计思想，实现的基于多 Reactor 多线程模型的网络库。

  

项目已经实现了 Channel 模块、Poller 模块、事件循环模块、定时器模块，日志模块、线程池模块、一致性哈希轮询算法。

  

## 开发环境

  

* linux kernel version5.15.0-130-generic (ubuntu 22.04)

* g++ (Ubuntu 13.1.0-8ubuntu1~22.04) 13.1.0

* cmake version 3.22.1

  

## 并发模型

  

![image.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1670853134528-c88d27f2-10a2-46d3-b308-48f7632a2f09.png?x-oss-process=image%2Fresize%2Cw_937%2Climit_0)

  
项目采用经典的主从多 Reactor 并发模型：

    Main Reactor (主循环): 运行在主线程，通过 Acceptor 专门负责监听和接受新的客户端连接。

    Sub Reactors (从循环): 运行在独立的 I/O 线程池中。Main Reactor 接受新连接后，通过轮询算法将其分发给一个 Sub Reactor。

    线程归属 (one loop per thread): 一个 TcpConnection 的整个生命周期（包括所有 I/O 事件、业务回调）都由同一个 Sub Reactor 线程负责，杜绝了跨线程的锁竞争，最大化地利用了 CPU 缓存局部性，是现代高性能网络服务的基石。

  

## 功能介绍

  

- **事件轮询与分发模块**：`EventLoop.*`、`Channel.*`、`Poller.*`、`EPollPoller.*`负责事件轮询检测，并实现事件分发处理。`EventLoop`对`Poller`进行轮询，`Poller`底层由`EPollPoller`实现。

- **线程与事件绑定模块**：`Thread.*`、`EventLoopThread.*`、`EventLoopThreadPool.*`绑定线程与事件循环，完成`one loop per thread`模型。

- **网络连接模块**：`TcpServer.*`、`TcpConnection.*`、`Acceptor.*`、`Socket.*`实现`mainloop`对网络连接的响应，并分发到各`subloop`。

- **缓冲区模块**：`Buffer.*`提供自动扩容缓冲区，保证数据有序到达。

- **高性能定时器 (TimerQueue)**:基于 Linux 的 timerfd 和 std::set (红黑树) 模仿Linux的CFS 调度算法，设计并实现了一个高精度、高效率的定时器模块。实现了线程安全的 runAfter, runEvery 接口，并设计了可取消 (cancel) 的 Timer 机制。


## 优化方向

- 在当前 TCP 库的基础上，构建 HttpContext, HttpRequest, HttpResponse 等核心类，实现一个完整的 HTTP 解析器和服务器框架 (HttpServer)。

- 集成高性能内存池，将自制的内存池作为静态库集成到项目中。通过重构 Buffer 类的底层内存管理，利用内存池的 ThreadCache 无锁分配特性，进一步优化高并发下的 I/O 吞吐性能。

- 全面拥抱lambda,遵循 Effective Modern C++ 的建议，将bind全面替换成lambda。

- 设置多主节点，提高可用性。

- 修改日志系统为异步。

- 增加更多的应用层测试用例,如RPC。

- 可以考虑引入协程库等模块



## 构建项目

  

安装基本工具

  

```shell

sudo apt-get update

sudo apt-get install -y wget cmake build-essential unzip git

```

  
  

## 编译指令

  

下载项目

  

```shell

git clone https://github.com/amikellt911/MiniMuduo.git

```

  

进入到MiniMuduo文件

```shell

cd MiniMuduo

```

  

创建build文件夹，并且进入build文件夹，并创建log文件夹:

```shell

mkdir build && cd build &&mkdir log

```

  

然后生成可执行程序文件：

```shell

cmake .. && make

```

  

执行可执行程序

```shell

./testserver

```

telnet 连接服务器
```shell
telnet 127.0.0.1 8080
```
  



## 致谢

  

- [作者-Shangyizhou]https://github.com/Shangyizhou/A-Tiny-Network-Library/tree/main

- [作者-S1mpleBug]https://github.com/S1mpleBug/muduo_cpp11?tab=readme-ov-file

- [作者-chenshuo]https://github.com/chenshuo/muduo

- 《Linux高性能服务器编程》

- 《Linux多线程服务端编程：使用muduo C++网络库》
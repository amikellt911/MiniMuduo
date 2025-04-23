# Channel 类学习笔记

本文档记录了在学习 `Channel.h` 过程中遇到的疑问点和对应的理解。

## 1. 回调函数与 `Timestamp`

### 疑问点

为什么只有读事件回调 `ReadEventCallback` 需要传入 `Timestamp` 参数，而其他事件回调（写、关闭、错误）不需要？

```cpp
using EventCallback = std::function<void()>;
// 为什么读事件要传入Timestamp？其他事件不用
using ReadEventCallback = std::function<void(Timestamp)>;
```

### 理解

*   **记录数据到达时间**：`Timestamp` 参数表示 `Poller` 检测到读事件发生的精确时间点。
*   **用途**：
    *   **日志记录**：精确记录收到数据的时间。
    *   **性能分析**：计算处理延迟。
    *   **业务逻辑**：处理有时效性的消息。
*   **其他事件**：写、关闭、错误事件通常更关注事件本身的状态或事实，而非发生的精确时间点，为了性能和简洁性，没有传递 `Timestamp`。

## 2. `tie_` 机制与生命周期管理

### 疑问点

`tie_` 和 `tie()` 方法的作用是什么？为什么需要这个机制？为什么使用 `weak_ptr` 而不是 `shared_ptr`？

```cpp
// 防止当channel被手动remove掉 channel还在执行回调操作
// 防止连接关闭，channel还在执行回调操作
// 将原本的shared_ptr存储为weak_ptr（没有引用计数） 防止循环引用，并且可以观察到对象是否存在
// 因为TcpConnection会拥有Channel，如果tie_也强引用TcpConnection,会导致两个对象无法被正常销毁，造成内存泄露
// 并且shared_ptr表示共享所有权，Channel不应该拥有，而是被拥有
// 使用weak_ptr表示弱引用，不共享所有权，不增加引用计数，可以观察到对象是否存在
// 在需要使用的时候，再通过tie_.lock()临时提升权限，处理事件，并且这是临时变量，可以精确控制，更加安全。
void tie(const std::shared_ptr<void> &);

std::weak_ptr<void> tie_;
bool tied_;
```

### 理解

*   **目的**：解决在 `Channel` 执行回调函数期间，其所有者对象（如 `TcpConnection`）可能被销毁，导致回调函数访问悬空指针的问题。
*   **工作原理**：
    1.  `tie()` 方法接收一个指向所有者对象的 `shared_ptr`，并将其存储为 `weak_ptr` (`tie_`)。
    2.  在 `handleEvent` 执行实际回调前，尝试通过 `tie_.lock()` 将 `weak_ptr` 提升为一个临时的 `shared_ptr` (`guard`)。
    3.  如果提升成功（对象还存活），`guard` 会临时增加对象的引用计数，保证在回调执行期间对象不会被销毁。
    4.  回调执行完毕后，`guard` 自动销毁，引用计数减少。
*   **为什么用 `weak_ptr`**：
    *   **避免循环引用**：`TcpConnection` 拥有 `Channel`，如果 `Channel` 也用 `shared_ptr` 强引用 `TcpConnection`，会形成循环引用，导致内存泄漏。
    *   **正确的所有权**：`Channel` 不拥有 `TcpConnection`，`weak_ptr` 表达了这种非所有权关系。
    *   **按需保护**：只在处理事件时才临时提升为 `shared_ptr`，精确控制生命周期。

## 3. `index_` 状态标记

### 疑问点

`index_` 的具体作用是什么？它和 `Poller` 内部列表的关系？如何利用 `index_` 执行操作？

```cpp
// 是一个状态标记，不是索引
// 分别有kNew(-1),kAdded(1),kDeleted(2)。这三种状态
// 表示新的（未添加到Poller），已添加到Poller，已从Poller删除
int index() { return index_; }
void set_index(int idx) { index_ = idx; }
```

### 理解

*   **定义**：`index_` 是 `Channel` 在 `Poller` 中的**状态标记**，记录了 `Channel`（及其 `fd`）与 `Poller` 的关系。它**不是**用来在 `Poller` 内部数据结构中查找 `Channel` 的数组索引。
*   **状态值**：
    *   `kNew (-1)`：从未添加到 `Poller`。
    *   `kAdded (1)`：已添加到 `Poller` 并正在监听。
    *   `kDeleted (2)`：之前添加过，但现在已从 `Poller` 移除（或标记为待移除）。
*   **作用**：当 `EventLoop` 要求 `Poller` 更新 (`updateChannel`) 或移除 (`removeChannel`) 一个 `Channel` 时，`Poller` 会：
    1.  接收到 `Channel` 对象的指针。
    2.  检查该 `Channel` 的 `index_` 状态。
    3.  根据 `index_` 的状态，决定调用哪个底层 I/O 多路复用操作（如 `epoll_ctl` 的 `ADD`, `MOD`, `DEL`），避免无效或重复的操作，提高效率。
    4.  操作成功后，更新 `Channel` 的 `index_` 状态。

## 4. `update()` 和 `remove()` 机制

### 疑问点

`update()` 和 `remove()` 如何与 `EventLoop` 和 `Poller` 交互？

```cpp
// update内部调用loop_->updateChannel(this);
// 通过loop_中的Poller和Channel的this指针，找到Poller维护的ChannelList中的Channel
// Poller会利用Channel的index_状态执行操作
// index_状态有三种：kNew(-1),kAdded(1),kDeleted(2)
// 1.kNew: 表示Channel未添加到Poller中，Poller检查channel是否有关系的事件，有则添加到Poller中，然后修改index_为kAdded
// 2.kAdded: 表示Channel已添加到Poller中，Poller检查channel是否有关系的事件，有则修改index_为kAdded，没有则修改index_为kDeleted
// 3.kDeleted: 表示Channel已从Poller中删除，Poller检查channel是否有关系的事件，有则添加到Poller中，然后修改index_为kAdded
void enableReading() { events_ |= kReadEvent; update(); }
// ... 其他 enable/disable 方法 ...
void remove();
private:
    void update();
```

### 理解

*   **流程**：
    1.  `Channel` 的 `enable/disable/remove` 等方法被调用。
    2.  这些方法内部调用 `update()` 或 `remove()`。
    3.  `update()` 调用 `loop_->updateChannel(this)`。
    4.  `remove()` 调用 `loop_->removeChannel(this)`。
    5.  `EventLoop` 将请求转发给其内部的 `Poller` 对象：`poller_->updateChannel(channel)` 或 `poller_->removeChannel(channel)`。
    6.  `Poller` 根据传入 `Channel` 的 `index_` 状态执行相应的底层操作（见上一节 `index_` 的解释）。
*   **关键点**：`Channel` 不直接操作 `Poller`，而是通过其所属的 `EventLoop` (`loop_`) 作为中介来完成。

## 5. 事件常量与位图

### 疑问点

`kNoneEvent`, `kReadEvent`, `kWriteEvent` 这些常量的具体值和意义是什么？

```cpp
// 看了源码epoll.h，其实就是bitmap
/*
 EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
 EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
 EPOLLOUT = 0x004,
 */
static const int kNoneEvent;
static const int kReadEvent;
static const int kWriteEvent;
```

### 理解

*   **本质**：这些常量实际上是对底层 I/O 多路复用事件（如 `epoll` 的 `EPOLLIN`, `EPOLLOUT`）的封装。
*   **位图 (Bitmap)**：它们的值通常被设计成 2 的幂（如 1, 2, 4, 8...），这样可以使用位运算（`|`, `&`, `~`）来组合和检查多个事件状态。
    *   `events_ |= kReadEvent;` // 添加读事件关注
    *   `events_ &= ~kWriteEvent;` // 移除写事件关注
    *   `if (revents_ & kReadEvent)` // 检查是否发生了读事件
*   **映射关系 (通常)**：
    *   `kNoneEvent` 可能为 0。
    *   `kReadEvent` 可能映射到 `EPOLLIN | EPOLLPRI`。
    *   `kWriteEvent` 可能映射到 `EPOLLOUT`。
    *   `handleEvent` 内部会处理 `EPOLLERR`, `EPOLLHUP` 等其他事件，即使它们没有定义为 `kXXXEvent` 常量。


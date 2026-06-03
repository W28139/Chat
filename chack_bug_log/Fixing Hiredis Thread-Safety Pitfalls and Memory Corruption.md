# 聊天服务器压测崩溃与性能优化

## 1. 问题 

在对聊天服务器项目进行高并发压测时（模拟 6 台服务器、10 线程、10,000 个登录请求），系统出现严重异常：

*   **稳定性极差**：多台服务器频繁崩溃，控制台报出以下错误：
    *   `Segmentation fault (core dumped)`
    *   `double free or corruption (fasttop)`
    *   `free(): corrupted unsorted chunks`
*   **性能表现惨淡**：
    *   **QPS 仅为 34**（远低于 C++ 网络服务器正常水平）。
    *   **错误率高**：失败/冲突请求占比高达 16%。

---

## 2. 定位

为了找到崩溃的根本原因，我采用了 **GDB** 和 **AddressSanitizer (ASan)** 组合拳。

### 2.1 开启 Core Dump 与 GDB

通过 `bt` (backtrace) 命令，发现崩溃点集中在 `hiredis` 的内部处理逻辑以及 Muduo 库的 `handleRead` 回调中。但这只是“结果”，并没有告诉我是谁破坏了内存。

### 2.2 使用 AddressSanitizer 定位

在 `CMakeLists.txt` 中开启 ASan 重新编译并压测，程序立刻抛出了详细报告：

```text
ERROR: AddressSanitizer: heap-use-after-free
freed by thread T3 (ChatServer2) here:
    #1 0x7557eb262c6f in sdsMakeRoomFor /root/hiredis/sds.c:226
previously allocated by thread T4 (ChatServer3) here:
    #2 0x7557eb262c6f in sdsMakeRoomFor /root/hiredis/sds.c:226
```

![](C:\Users\28783\Desktop\jupyter notebook\项目\集群聊天服务器\pc\21.png)

**关键发现**：不同线程（T3 和 T4）在同一时间操作了同一个内存区域。

---

## 3. 分析

通过 ASan 报告和源码对比，确定了问题的根源：**Redis 客户端（Hiredis）的非线程安全访问**。

1.  **竞态条件 (Race Condition)**：
    *   项目中只维护了单实例的 `redisContext`。
    *   Muduo 网络库是多线程的，`onMessage` 会在多个 EventLoop 线程中触发。
    *   当多个线程同时调用 `redisCommand` 时，Hiredis 内部的缓冲区（sds）会发生重分配（realloc）。
    *   **线程 A** 释放了旧内存块，**线程 B** 却仍在向旧地址写入，导致 `heap-use-after-free`。

2.  **性能瓶颈原因**：
    *   由于内存损坏导致的底层异常，使得大量请求在 Redis 层阻塞或失败。
    *   由于没有使用连接池，高并发下单个 Redis 连接成为了系统的“独木桥”，导致 QPS 只有的 34。

---

## 4. 解决问题

针对上述原因，我实施了**连接池** 改造方案。

### 4.1 引入发布连接池

不再让所有线程抢夺一个连接，而是预先创建一组连接放入池中。

*   **动作**：使用 `std::queue` 存储 `redisContext*`，并配合 `std::mutex` 和 `std::condition_variable` 进行同步。
*   **效果**：实现并发发布消息。每个线程取出一个独立 Context 执行命令，执行完后归还，彻底消除了 `publish` 时的内存竞争。

### 4.2 订阅连接安全化

由于 `subscribe` 涉及一个长期的监听线程（observer），我采取了加锁保护。

*   **动作**：在执行 `subscribe` 和 `unsubscribe` 命令时，对 `_subcribe_context` 加互斥锁，确保写操作不会与 `observer` 线程的读操作冲突。

### 4.3 核心代码片段 (优化后)

```cpp
// 从连接池获取连接进行发布，避免线程冲突
bool Redis::publish(int channel, std::string message) {
    redisContext *ctx = getContextFromPool(); // 加锁取连接
    redisReply *reply = (redisReply *)redisCommand(ctx, "PUBLISH %d %s", ...);
    freeReplyObject(reply);
    returnContextToPool(ctx); // 归还连接
    return true;
}
```

---

## 5. 总结

###  经验总结

1.  **内存安全优先**：在 C++ 多线程开发中，绝不能假设第三方 C 库（如 Hiredis, MySQL API）是线程安全的。
2.  **工具的重要性**：
    *   `GDB` 用于查看崩溃现场。
    *   `ASan` 用于追踪内存被破坏的过程。
3.  **连接池是高并发标配**：无论是 Redis 还是 MySQL，单连接在多线程环境下既不安全也是性能杀手，必须使用连接池。







在原版实现中，`Redis` 类仅持有两个固定的 `redisContext`：一个用于发布（Publish），一个用于订阅（Subscribe）。

### ❌ 致命缺陷：非线程安全导致的崩溃

*   **现象**：在高并发压测下，系统报出 `Segmentation fault` 或 `double free`。
*   **原因**：Hiredis 库底层的 `redisContext` **不是线程安全**的。
*   **深层原理**：Muduo 网络库是在多线程（EventLoop 线程池）中运行的。当多个用户同时发送消息时，多个线程会同时进入 `Redis::publish` 函数并调用 `redisCommand`。
    *   Hiredis 内部会维护一个发送/接收缓冲区（SDS 字符串）。
    *   **竞态条件**：线程 A 正在进行缓冲区扩容（realloc）并释放旧内存，线程 B 同时在操作这块旧内存。
    *   **结果**：AddressSanitizer 检测到 `heap-use-after-free`，程序直接崩溃。

### ❌ 性能瓶颈

*   **现象**：6 台服务器 QPS 总计仅为 **34**。
*   **原因**：由于 Hiredis 的同步阻塞特性，单连接在同一时间只能处理一个请求。在高并发下，所有线程都在排队等待这唯一的一个 Redis 连接，导致 CPU 绝大部分时间处于空转等待 IO 的状态。

---

## 2. 什么是连接池？(What is a Connection Pool?)

### 💡 基本概念

**连接池 (Connection Pool)** 是一种资源管理技术。它预先创建并维护多个数据库连接，将它们存放在一个“池子”（容器）里。

*   **工作流程**：
    1.  **初始化**：程序启动时，预先建立 N 个连接。
    2.  **获取 (Pop)**：当业务线程需要操作 Redis 时，从池子中“借”走一个空闲连接。
    3.  **使用**：线程独占该连接进行通信，其他线程无法干扰。
    4.  **归还 (Push)**：操作完成后，线程将连接“还”回池子，供后续请求复用。
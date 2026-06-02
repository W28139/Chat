# ClusterChatServer (集群聊天服务器)

Chat 是一个基于 **C++ 11** 开发的高并发分布式聊天服务器。项目采用 **muduo** 网络库作为底层网络核心，利用 **Redis 的发布/订阅机制** 实现了跨服务器的消息推送，并结合 **Nginx 的 TCP 负载均衡** 实现了服务器集群的横向扩展。

[![C++](https://img.shields.io/badge/Language-C++11-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/)
[![Database](https://img.shields.io/badge/Database-MySQL-orange.svg)](https://www.mysql.com/)

**之后会不断完善内容，业务功能方面向WeChat靠拢模仿**
---

## 项目特性

-   **高并发网络模型**：基于 `muduo` 网络库，采用 `Reactors` 多线程模型（非阻塞 I/O + 事件驱动）。
-   **集群横向扩展**：支持通过 `Nginx (TCP Stream)` 进行负载均衡，动态增减服务器节点。
-   **跨服务器通信**：通过 `Redis` 的 `Publish/Subscribe` 机制实现不同服务器上的用户实时聊天。
-   **可靠的业务功能**：
    -   用户注册、登录、注销。
    -   点对点单聊（支持在线/离线消息存储）。
    -   好友管理（添加好友、查询好友状态）。
    -   群组功能（创建群组、加入群组、群聊）。
-   **解耦设计**：
    -   **业务逻辑与网络层分离**：`ChatServer` 负责网络 I/O，`ChatService` 负责业务逻辑。
    -   **数据持久化**：采用 `Model` 层封装数据库操作，类似 ORM 的思想。

---

## 系统架构

```text
                  [ 客户端 ]   [ 客户端 ]   [ 客户端 ]
                       \          |          /
                        \         |         /
                      [ Nginx TCP 负载均衡 ]
                      /          |          \
            [ Server 1 ] --- [ Redis ] --- [ Server 2 ]
                 |            (Pub/Sub)          |
            [ MySQL ] ----------------------- [ MySQL ]
```

---

## 技术栈

-   **网络库**：`muduo` (基于 Epoll)
-   **负载均衡**：`Nginx` (Stream 模块)
-   **中间件**：`Redis` (hiredis 客户端)
-   **数据库**：`MySQL 5.7`
-   **序列化**：`JSON for Modern C++`
-   **开发环境**：`Linux (Ubuntu)` / `CMake` / `G++`

---

## 项目结构描述

```bash
├── bin/                  # 存放编译生成的二进制可执行文件
├── build/                # CMake 编译生成的中间文件
├── include/              # 头文件目录
│   ├── server/           # 服务端核心头文件
│   │   ├── db/           # 数据库操作模块
│   │   │   └── db.h
│   │   ├── model/        # 数据模型层 (DAO)，封装数据库业务逻辑
│   │   │   ├── friendmodel.hpp
│   │   │   ├── group.hpp
│   │   │   ├── groupmodel.hpp
│   │   │   ├── groupuser.hpp
│   │   │   ├── offlinemessagemodel.hpp
│   │   │   ├── user.hpp
│   │   │   └── UserModel.hpp
│   │   ├── redis/        # Redis 跨服务器通信模块
│   │   │   └── redis.hpp
│   │   ├── chatserver.hpp    # 网络层封装
│   │   └── chatservice.hpp   # 业务层封装
│   └── public.hpp        # 服务端与客户端公共消息类型定义
├── src/                  # 源代码目录
│   ├── client/           # 客户端代码实现
│   └── server/           # 服务端源代码实现 (与 include 中的 .hpp 对应)
├── test/                 # 单元测试与压力测试脚本
├── thirdparty/           # 第三方库 (如 nlohmann/json)
├── CMakeLists.txt        # 项目顶层 CMake 构建脚本
└── README.md
```

---

## 数据库设计

项目主要包含以下核心表结构:

1.  **User 表**：存储用户基本信息及在线状态。
2.  **Friend 表**：存储用户好友关系。
3.  **AllGroup 表**：存储群组基本信息。
4.  **GroupUser 表**：存储群成员信息及角色。
5.  **OfflineMessage 表**：存储离线用户未读的消息。

---

## 核心逻辑说明

### 1. 跨服务器通信 (Redis)
当 `Server 1` 上的 `User A` 给 `Server 2` 上的 `User B` 发消息时：
1. `Server 1` 发现 `User B` 不在本服务器。
2. `Server 1` 向 Redis 通道 `B的ID` 发布消息。
3. `Server 2` 订阅了该通道，收到通知后将消息推送到 `User B` 的客户端。

### 2. 负载均衡 (Nginx)
在 `nginx.conf` 中配置 `stream` 模块：
```nginx
stream {
    upstream MyChatCluster {
        server 127.0.0.1:6000 weight=1;
        server 127.0.0.1:6002 weight=1;
    }
    server {
        listen 8000;
        proxy_pass MyChatCluster;
    }
}
```

---

## 构建与运行

### 1. 准备工作
确保已安装 `muduo`, `hiredis`, `mysql-client`, `cmake`, `nlohmann-json`。

### 2. 编译
```bash
mkdir build
cd build
cmake ..
make
```

### 3. 运行
```bash
# 启动 Redis 和 MySQL
# 启动服务器1
./ChatServer 127.0.0.1 6000
# 启动服务器2
./ChatServer 127.0.0.1 6002
```

---

## 消息接口协议 (JSON)
本项目通过消息头中的 `msgid` 来识别业务类型：

| msgid (EnMsgType) | 描述 | 关键字段 |
| :--- | :--- | :--- |
| 1 | LOGIN_MSG | id, password |
| 3 | REG_MSG | name, password |
| 5 | ONE_CHAT_MSG | toid, msg |
| 7 | CREATE_GROUP_MSG | groupname, groupdesc |

---

## 贡献
欢迎提交 Issue 或 Pull Request 来完善此项目！

---
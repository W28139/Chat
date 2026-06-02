#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>

#include"json.hpp"
using json = nlohmann::json;

using namespace muduo;
using namespace muduo::net;

// 聊天服务器的主类
class ChatServer
{
public:
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg
        );

    void start();
private:
    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    // 组合的muduo库，实现服务器功能的类的对象
    TcpServer _server;
    // 事件循环
    EventLoop *_loop;
};


#endif
#include "chatserver.hpp"
#include "chatservice.hpp"
#include<muduo/base/Logging.h>

ChatServer::ChatServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const string& nameArg
            ):_server(loop,listenAddr,nameArg)
            ,_loop(loop)
{
    // 处理回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,std::placeholders::_1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    ));
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        // 处理异常退出
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer,Timestamp time)
{   
    string buf = buffer->retrieveAllAsString();
    try
    {
        json js = json::parse(buf);
        auto MsgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
        MsgHandler(conn,js,time);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR <<"JSON解析失败:"<<e.what();
        conn->shutdown();
    }
    
}


// void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer,Timestamp time)
// {   
//     // 从buffer缓冲中拿出数据放到buf里
//     string buf = buffer->retrieveAllAsString();

//     // 数据的反序列化(解码)
//     json js = json::parse(buf);

//     // 通过js['msgid']获取 -> 业务处理方式 ==》conn js time
//     auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
//     // 回调消息绑定好的事件处理器，来执行相应的业务处理
//     // 这里回调函数需要传入3个参数，如chatservice业务模块中写入的_1,_2,_3,具体做的事件是传入的函数 login,reg等
//     msgHandler(conn,js,time);

// }
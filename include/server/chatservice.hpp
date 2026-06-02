#ifndef CHATSERVICE_H
#define CHATSERVICE_H

// 实现一个消息ID映射一个事件处理(回调)
#include<unordered_map>
#include<functional>
#include<mutex>
#include<string>
#include<iostream>
#include<mutex>
#include<vector>
#include<map>

#include"json.hpp"
#include"public.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"offlinemessagemodel.hpp"
#include"UserModel.hpp"
#include"redis.hpp"

#include<muduo/net/TcpConnection.h>
#include<muduo/base/Logging.h>

// 获取单例对象的接口函数
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;


using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &js,Timestamp)>;


// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js ,Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);

    // 一对一两天业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);

    // 获取消息对应的一个处理器
    MsgHandler getHandler(int msgid);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js ,Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理redis回调业务
    void handleRedisSubscribeMessage(int userid,std::string msg);

private:
    ChatService();

    // 存储消息ID 和 对应业务的处理方法
    std::unordered_map<int,MsgHandler> _msgHandlerMap;

    // 数据操作类对象
    UserModel _userModel;

    // 定义互斥锁，保证_userConnMap的线程安全
    std::mutex _connMutex;
    // 存储在线用户的通信连接
    std::unordered_map<int,TcpConnectionPtr> _userConnMap;

    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendmodel;
    GroupModel _groupModel;

    Redis _redis;
};

#endif
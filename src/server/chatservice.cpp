#include"chatservice.hpp"


ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,
                            std::bind(&ChatService::login,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );

    _msgHandlerMap.insert({REG_MSG,
                            std::bind(&ChatService::reg,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );
    _msgHandlerMap.insert({ONE_CHAT_MSG,
                            std::bind(&ChatService::oneChat,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );
    _msgHandlerMap.insert({ADD_FRIEND_MSG,
                            std::bind(&ChatService::addFriend,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );
    _msgHandlerMap.insert({CREATE_GROUP_MSG,
                            std::bind(&ChatService::createGroup,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );
    _msgHandlerMap.insert({ADD_GROUP_MSG,
                            std::bind(&ChatService::addGroup,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );
    _msgHandlerMap.insert({GROUP_CHAT_MSG,
                            std::bind(&ChatService::groupChat,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );
    _msgHandlerMap.insert({LOGINOUT_MSG,
                            std::bind(&ChatService::loginout,this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3)
                            }
                        );

    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,std::placeholders::_1,std::placeholders::_2));
    }

    // 服务器启动，所有用户状态重置为offline
    _userModel.resetState();

}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调——直接用muduo的日志系统
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        // 进行空操作避免程序终止，并且打印日志进行提示
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id = js["id"].get<int>();
    std::cout<<id<<std::endl;
    std::string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() ==id && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            // 用户已经登陆。不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登陆";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功
            // 记录用户连接信息
            {
                std::lock_guard<std::mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }
            // 登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            user.setState("online");
            _userModel.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            
            // 查询是否有离线消息
            std::vector<std::string> msgs = _offlineMsgModel.query(id);
            if(!msgs.empty())
            {
                response["offlinemsg"] = msgs;
                _offlineMsgModel.remove(id);
            }

            // 查询该用户好友信息并返回
            std::vector<User> userVec = _friendmodel.query(id);
            if(!userVec.empty())
            {
                std::vector<std::string> vec;
                for(User &user:userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec.push_back(js.dump());
                }
                response["friends"]=vec;
            }

            // 查询该用户群组信息并返回
            std::vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                // groupV 存储所有群组的 JSON 字符串
                std::vector<std::string> groupV;
                for (Group &group : groupVec)
                {
                    json grpjs;
                    grpjs["id"] = group.getId();
                    grpjs["groupname"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();

                    // userV 存储该群组内所有成员的 JSON 字符串
                    std::vector<std::string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjs["users"] = userV;
                    groupV.push_back(grpjs.dump());
                }
                // 将整个群组信息数组放入 response 响应包中
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else if(user.getId()!=id)
    {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户不存在";
        conn->send(response.dump());
    }
    else if(user.getPwd() != pwd)
    {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "密码错误";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    std::string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if(state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        std::lock_guard<std::mutex> lock(_connMutex);

        // 先在本台主机上面找一下，看看在不在线
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            // toid在线，转发消息,直接转就好
            it->second->send(js.dump());
            return;
        }
    }
    // toid不在线 或者 在其他主机
    User user = _userModel.query(toid);
    // 如果toid在其他主机上在线，那将消推到Redis的channels里
    if(user.getState()=="online")
    {
        _redis.publish(toid,js.dump());
        return;
    }
    else
    {
        // 如果不在线，就存储离线消息
        _offlineMsgModel.insert(toid,js.dump());
    }
}


// 目前一个线程只能设置一个账号，理论上也只能这样
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        std::lock_guard lock(_connMutex);
        for(auto it = _userConnMap.begin();it != _userConnMap.end();it++)
        {
            if(it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
           }
        }
    }
    _redis.unsubscribe(user.getId());
    if(user.getId()!=-1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendmodel.insert(userid,friendid);
}


void ChatService::createGroup(const TcpConnectionPtr &conn,json &js ,Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    for (int id : useridVec)
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 存储离线群消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}


void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        std::lock_guard<std::mutex>lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it!=_userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    User user(userid,"","","offline");
    _userModel.updateState(user);
    _redis.unsubscribe(userid);
}

void ChatService::handleRedisSubscribeMessage(int userid,std::string msg)
{
    {
        std::lock_guard<std::mutex>lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it!=_userConnMap.end())
        {
            it->second->send(msg);
            return;
        }
    }
    // 调用该函数后一瞬间下线了，那就存储离线消息
    _offlineMsgModel.insert(userid,msg);
}
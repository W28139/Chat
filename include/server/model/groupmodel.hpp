// 群组的数据库操作接口方法

#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include"group.hpp"
#include<string>
#include<vector>

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 将用户加入组
    void addGroup(int userid, int groupid, std::string role);
    // 查询用户所在群组信息
    std::vector<Group> queryGroups(int userid);
    // 用于群聊，用户userid在groupid里，发送群消息而使用到的其他用户id(除去userid，避免自己给自己发消息)
    std::vector<int>queryGroupUsers(int userid,int groupid);
};


#endif
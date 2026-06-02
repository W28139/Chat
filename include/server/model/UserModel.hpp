
#ifndef UserModel_h
#define uUserModel_h

#include"user.hpp"
// 针对表的方法，与业务无关
// User表的数据操作类(提供方法的)
class UserModel
{
public:
    // User表的增加方法
    bool insert(User &user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 更新用户的状态信息
    bool updateState(User user);

    void resetState();

};

#endif
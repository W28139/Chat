#ifndef GROUPUSER_H
#define GROUPUSER_H
#include"user.hpp"
// 相比于user ,只是添加了一个用户角色信息，因此单独设没groupuser

class GroupUser : public User
{
public:
    void setRole(std::string role) {this-> role = role;}
    std::string getRole(){return this->role;}

private:
    std::string role;
}; 


#endif
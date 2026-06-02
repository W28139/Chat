#ifndef db_h
#define db_h

#include <iostream>
#include <string>
#include <mysql/mysql.h> // 要链接 mysqlclient 库

// 数据库配置信息
static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

// 数据库操作类
class MySQL 
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect();

    // 更新接口：增加、删除、修改 (Insert, Update, Delete)
    bool update(const std::string &sql);

    // 查询接口：查询数据 (Select)
    // 在实际商业项目中，通常会返回一个自定义的数据集结构，这里为了演示，直接打印输出结果
    MYSQL_RES* query(const std::string &sql);

    // 获取连接
    MYSQL* getConnection();
    
private:
    MYSQL *_conn; // 数据库连接句柄
};

#endif
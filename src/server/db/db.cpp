#include"db.h"

MySQL::MySQL() 
{
    _conn = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL() 
{
    if (_conn != nullptr) 
    {
        mysql_close(_conn);
    }
}

bool MySQL::connect() 
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr) 
    {
        // C/C++ 默认编码为ASCII
        // 改为：utf8mb4，支持完整的 UTF-8 字符集（包括表情符号等）
        mysql_query(_conn, "set names utf8mb4");
        std::cerr << "数据库连接成功: " << std::endl;
        return true;
    }
    else 
    {
        std::cerr << "数据库连接失败: " << mysql_error(_conn) << std::endl;
        return false;
    }
}

bool MySQL::update(const std::string &sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        std::cerr << "更新失败，SQL: " << sql << "，错误原因: " << mysql_error(_conn) << std::endl;
        return false;
    }
    return true;
}

MYSQL_RES* MySQL::query(const std::string &sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        std::cerr << "查询失败，SQL: " << sql << "，错误原因: " << mysql_error(_conn) << std::endl;
        return nullptr;
    }
    // mysql_use_result 或 mysql_store_result 用于获取查询结果
    return mysql_store_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    return _conn;
}

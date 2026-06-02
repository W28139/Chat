#include"offlinemessagemodel.hpp"
#include"db.h"

void OfflineMsgModel::insert(int userid,std::string msg)
{
    char sql[1024] = {0};
    sprintf(sql,"insert into Offlinemessage values('%d','%s')",userid,msg.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

void OfflineMsgModel::remove(int userid)
{
    char sql[1024]={0};
    sprintf(sql,"delete from Offlinemessage where userid = %d",userid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

std::vector<std::string> OfflineMsgModel::query(int userid)
{
    char sql[1024]={0};
    sprintf(sql,"select message from Offlinemessage where userid = %d",userid);

    std::vector<std::string>a;
    MySQL mysql;
    if(mysql.connect())
    {
        // 查询拿到表
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            // 循环拿表的每一行
            while((row = mysql_fetch_row(res))!=nullptr)
            {
                a.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return a;
}
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "json.hpp" // 确保你有这个头文件

/*
g++ stress_test.cpp -o stress_test -lpthread -std=c++11
*/
using json = nlohmann::json;
using namespace std;

// 对应你代码中的消息 ID 约定
enum MsgType {
    LOGIN_MSG = 1,    // 登录
    LOGIN_MSG_ACK,    // 登录确认
    REG_MSG,          // 注册
    REG_MSG_ACK,      // 注册确认
    ONE_CHAT_MSG,     // 私聊
};

// 配置信息
const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8000;
const int THREAD_COUNT = 15;        // 并发线程数
const int REQUESTS_PER_THREAD = 200; // 每个线程模拟的用户数
const int START_ID = 1;            // 压测起始 ID (确保数据库里有这些 ID)

atomic<int> success_count(0);
atomic<int> failure_count(0);

// 辅助函数：构造登录 JSON
string get_login_json(int id) {
    json js;
    js["msgid"] = LOGIN_MSG;
    js["id"] = id;
    js["password"] = "123456"; // 假设密码统一
    return js.dump();
}

// 压测逻辑
void worker(int start_id, int count) {
    for (int i = 0; i < count; ++i) {
        int current_id = start_id + i;

        // 1. 创建并连接
        int clientfd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(clientfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            failure_count++;
            close(clientfd);
            continue;
        }

        // 2. 发送登录消息
        string send_str = get_login_json(current_id);
        if (send(clientfd, send_str.c_str(), send_str.size(), 0) <= 0) {
            failure_count++;
            close(clientfd);
            continue;
        }

        // 3. 接收登录响应
        char buffer[2048] = {0};
        int len = recv(clientfd, buffer, 2048, 0);
        if (len > 0) {
            try {
                json response = json::parse(buffer);
                if (response["errno"].get<int>() == 0) {
                    // 登录成功
                    success_count++;
                    
                    // --- 可选：模拟发送一条私聊消息 ---
                    
                    json chatjs;
                    chatjs["msgid"] = ONE_CHAT_MSG;
                    chatjs["from"] = current_id;
                    chatjs["to"] = 2; // 发给 ID 为 2 的人
                    chatjs["msg"] = "hello stress test";
                    string cstr = chatjs.dump();
                    send(clientfd, cstr.c_str(), cstr.size(), 0);
                    
                } 
                else {
                    // 可能是账号已登录或密码错误
                    failure_count++;
                }
            } catch (...) {
                failure_count++;
            }
        } else {
            failure_count++;
        }

        // 4. 关闭连接 (模拟短连接压测)
        close(clientfd);
    }
}

int main() {
    cout << "ChatServer 压测开始..." << endl;
    auto start_time = chrono::high_resolution_clock::now();

    vector<thread> threads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        // 每个线程分配不同的 ID 范围，防止重复登录冲突
        threads.emplace_back(worker, START_ID + (i * REQUESTS_PER_THREAD), REQUESTS_PER_THREAD);
    }

    for (auto& t : threads) t.join();

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> diff = end_time - start_time;

    cout << "--------------------------------" << endl;
    cout << "总请求数: " << THREAD_COUNT * REQUESTS_PER_THREAD << endl;
    cout << "成功登录: " << success_count << endl;
    cout << "失败/冲突: " << failure_count << endl;
    cout << "总耗时: " << diff.count() << "s" << endl;
    cout << "QPS: " << success_count / diff.count() << endl;
    
    return 0;
}
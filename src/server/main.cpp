#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器异常退出（如 Ctrl+C），重置用户在线状态
void resetHandler(int) {
    ChatService::instance();
    exit(0);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析命令行参数：IP 和 端口
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 注册信号处理函数
    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}
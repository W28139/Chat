#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器 
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, std::string message);

    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();

    // 初始化业务层上报消息的回调对象
    void init_notify_handler(std::function<void(int, std::string)> fn);

private:
    // 内部获取连接的方法
    redisContext* allocContext();

    // --- 连接池相关 ---
    std::queue<redisContext *> _publish_pool;
    std::mutex _pool_mutex;
    std::condition_variable _condition;
    int _pool_size; 

    // --- 订阅相关 ---
    redisContext *_subcribe_context;
    std::mutex _sub_mutex; // 保护 _subcribe_context 的读写安全

    // 业务层回调
    std::function<void(int, std::string)> _notify_message_handler;
};

#endif
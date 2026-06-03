#include "redis.hpp"
#include <iostream>
#include <sys/time.h>

Redis::Redis()
    : _pool_size(8), _subcribe_context(nullptr)
{
}

Redis::~Redis()
{
    // 释放连接池
    std::lock_guard<std::mutex> lock(_pool_mutex);
    while (!_publish_pool.empty()) {
        redisFree(_publish_pool.front());
        _publish_pool.pop();
    }

    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

redisContext* Redis::allocContext() {
    redisContext* ctx = redisConnect("127.0.0.1", 6379);
    if (nullptr == ctx || ctx->err) {
        if (ctx) redisFree(ctx);
        std::cerr << "connect redis failed!" << std::endl;
        return nullptr;
    }
    return ctx;
}

bool Redis::connect()
{
    // 1. 初始化发布消息的连接池
    for (int i = 0; i < _pool_size; ++i) {
        redisContext* ctx = allocContext();
        if (ctx) {
            _publish_pool.push(ctx);
        } else {
            return false;
        }
    }

    // 2. 初始化订阅消息的上下文
    _subcribe_context = allocContext();
    if (nullptr == _subcribe_context) return false;

    // --- 关键：设置订阅连接的超时时间，防止 observer 线程永久死锁 ---
    struct timeval timeout = {0, 100000}; // 100毫秒超时
    redisSetTimeout(_subcribe_context, timeout);

    // 3. 启动独立线程监听订阅消息
    std::thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    std::cout << "connect redis-server success with pool!" << std::endl;
    return true;
}

bool Redis::publish(int channel, std::string message)
{
    redisContext *ctx = nullptr;
    {
        std::unique_lock<std::mutex> lock(_pool_mutex);
        // 如果池空，等待最多1秒
        if (!_condition.wait_for(lock, std::chrono::seconds(1), [this] { return !_publish_pool.empty(); })) {
            std::cerr << "publish error: pool empty!" << std::endl;
            return false;
        }
        ctx = _publish_pool.front();
        _publish_pool.pop();
    }

    redisReply *reply = (redisReply *)redisCommand(ctx, "PUBLISH %d %s", channel, message.c_str());
    
    bool result = true;
    if (nullptr == reply)
    {
        std::cerr << "publish command failed!" << std::endl;
        redisFree(ctx);
        ctx = allocContext(); // 尝试重建连接
        result = false;
    }
    else
    {
        freeReplyObject(reply);
    }

    // 归还连接
    {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        _publish_pool.push(ctx);
        _condition.notify_one();
    }
    return result;
}

bool Redis::subscribe(int channel)
{
    std::lock_guard<std::mutex> lock(_sub_mutex);
    // 只写缓冲区，由 observer 线程的 redisGetReply 负责发出
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }
    return true;
}

bool Redis::unsubscribe(int channel)
{
    std::lock_guard<std::mutex> lock(_sub_mutex);
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }
    return true;
}

void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(_sub_mutex);
            // 尝试读取，如果 100ms 没消息会返回 REDIS_ERR
            if (redisGetReply(this->_subcribe_context, (void **)&reply) == REDIS_ERR)
            {
                // 超时或出错，释放锁，给 subscribe 线程执行机会
                continue; 
            }
        }

        // 此时已释放锁，处理消息逻辑
        if (reply != nullptr)
        {
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3)
            {
                if (reply->element[2] != nullptr && reply->element[2]->str != nullptr)
                {
                    _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
                }
            }
            freeReplyObject(reply);
            reply = nullptr;
        }
    }
}

void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->_notify_message_handler = fn;
}
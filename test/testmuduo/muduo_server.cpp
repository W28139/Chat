/*
muduo提供的接口类：
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

只需要书写业务
	: 用户的连接和断开
	：用户的可读写事件
*/

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace placeholders;
using namespace muduo::net;
using namespace muduo;

class ChatServer
{
public:
	ChatServer(EventLoop* loop,			// 事件循环
			const InetAddress& listenAddr,	// ip + port
			const string& nameArg)		// 服务器名字
	:_server(loop,listenAddr,nameArg),_loop(loop)
	{
		// 给下层服务器注册用户连接的创建与断开回调
		_server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

		// 给服务器注册用户读写事件回调
		_server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

		// 设置服务器端线程的数量
		_server.setThreadNum(4);	// 1个IO,3个work线程
	
	}

	void start()
	{
		_server.start();
	}	
	
private:
	TcpServer _server;
	EventLoop *_loop;
	
	// 专门处理用户连接与断开
	void onConnection(const TcpConnectionPtr& conn)
	{
		if(conn->connected())
		{
			cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<endl;
			cout<<"state:online"<<endl;
		}
		else
		{
			cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<endl;
			cout<<"state:offline"<<endl;
			conn->shutdown();	// close(fd)
			// _loop->quit();	// 退出epoll
		}
	}

	// 专门处理用户的读写事件
	void onMessage(const TcpConnectionPtr &conn,	// 连接
			Buffer *buffer,			// 缓冲区
			Timestamp time)			// 接收到数据的时间信息
	{
		string buf = buffer->retrieveAllAsString();
		cout<<"recv data:"<<buf<<"temp:"<<time.toString()<<endl;
		conn->send(buf);
	}
};

int main()
{
	EventLoop loop;
	InetAddress addr("127.0.0.1",6789);
	ChatServer server(&loop,addr,"ChatSever");

	server.start();	// 启动服务器，做好预处理等
	loop.loop();	// 开启事件循环，epoll_wait 以阻塞方式等待新用户连接、读写操作等

	return 0;
}

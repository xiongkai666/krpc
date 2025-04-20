#pragma once

#include <pthread.h>
#include <sys/socket.h>
#include <queue>
#include <string>

class KrpcConnectionPool {
   public:
    // 单例模式获取连接池实例
    static KrpcConnectionPool* GetInstance();

    // 初始化连接池，参数为服务地址和初始连接数
    bool Init(const std::string& serviceAddr, int initConnCount = 5);

    // 获取可用连接
    int GetConnection();

    // 释放连接回连接池
    void ReleaseConnection(int conn);

    // 销毁连接池
    ~KrpcConnectionPool();

private:
    KrpcConnectionPool(){}

    // 创建新连接
    int CreateNewConnection();

    std::string _serviceAddr;
    std::string _ip;
    uint16_t _port;
    std::queue<int> _connQueue;  // 空闲连接队列
    pthread_mutex_t _mutex;
    const int MAX_CONN_COUNT = 20;  // 最大连接数
    int _currentConnCount = 0;      // 当前总连接数
};
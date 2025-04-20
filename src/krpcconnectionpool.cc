#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "krpcconnectionpool.h"

// 单例实例实现
KrpcConnectionPool* KrpcConnectionPool::GetInstance() {
    static KrpcConnectionPool instance;
    return &instance;
}

// 初始化连接池
bool KrpcConnectionPool::Init(const std::string& serviceAddr, int initConnCount) {
    _serviceAddr = serviceAddr;
    int idx = serviceAddr.find(":");
    if (idx == -1) {
        return false;
    }
    _ip = serviceAddr.substr(0, idx);
    _port = static_cast<uint16_t>(atoi(serviceAddr.substr(idx + 1).c_str()));

    if (pthread_mutex_init(&_mutex, nullptr) != 0) {
        return false;
    }

    for (int i = 0; i < initConnCount; ++i) {
        int conn = CreateNewConnection();
        if (conn != -1) {
            _connQueue.push(conn);
        }
    }
    return true;
}

// 获取可用连接
int KrpcConnectionPool::GetConnection() {
    pthread_mutex_lock(&_mutex);
    int conn = -1;
    if (!_connQueue.empty()) {
        conn = _connQueue.front();
        _connQueue.pop();
    } else if (_currentConnCount < MAX_CONN_COUNT) {
        conn = CreateNewConnection();
        if (conn != -1) {
            _currentConnCount++;
        }
    }
    pthread_mutex_unlock(&_mutex);
    return conn;
}

// 释放连接回连接池
void KrpcConnectionPool::ReleaseConnection(int conn) {
    pthread_mutex_lock(&_mutex);
    _connQueue.push(conn);
    pthread_mutex_unlock(&_mutex);
}

// 销毁连接池（析构函数实现）
KrpcConnectionPool::~KrpcConnectionPool() {
    pthread_mutex_lock(&_mutex);
    while (!_connQueue.empty()) {
        int conn = _connQueue.front();
        _connQueue.pop();
        close(conn);
    }
    pthread_mutex_unlock(&_mutex);
    pthread_mutex_destroy(&_mutex);
}

// 创建新连接（私有方法实现）
int KrpcConnectionPool::CreateNewConnection() {
    int conn = socket(AF_INET, SOCK_STREAM, 0);
    if (conn == -1) {
        return -1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    if (inet_pton(AF_INET, _ip.c_str(), &server_addr.sin_addr) <= 0) {
        close(conn);
        return -1;
    }

    if (connect(conn, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(conn);
        return -1;
    }
    return conn;
}
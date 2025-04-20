#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <mutex>
#include <string>

// 封装的zk客户端类
class ZkClient
{
public:
    static ZkClient& GetInstance();//4.20
    ~ZkClient();
    // zkclient启动连接zkserver
    void Start();
    // 在zkserver上根据指定的path创建znode节点
    void Create(const char* path, const char* data, int datalen, int state = 0);
    // 根据参数指定的znode节点路径，获取znode节点的值
    std::string GetData(const char* path);
    void Reconnect();

private:
    // zk的客户端句柄
    zhandle_t *m_zhandle;
    ZkClient() = default;  // 4.20
    static ZkClient instance; //4.20
    std::mutex m_mutex;
};
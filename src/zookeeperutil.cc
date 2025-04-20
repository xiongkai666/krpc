#include "zookeeperutil.h"
#include <semaphore.h>
#include <chrono>
#include <thread>
#include <iostream>
#include "krpcapplication.h"
// 4.20
ZkClient ZkClient::instance;
ZkClient& ZkClient::GetInstance() {
    return instance;
}

// 全局的watcher观察器，zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (state == ZOO_EXPIRED_SESSION_STATE) {
        std::cout << "Session expired, attempting to reconnect..." << std::endl;
        ZkClient& zkCli = ZkClient::GetInstance();
        zkCli.Reconnect();
    }
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
		{
			sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
		}
	}
}

ZkClient::~ZkClient()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源
        m_zhandle = nullptr;
    }
}

// 连接
void ZkClient::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_zhandle)
        return;  // 已连接直接返回

    // 获取配置信息
    std::string host = KrpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = KrpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    // 初始化连接
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 15000, nullptr, nullptr, 0);
    if (!m_zhandle) {
        std::cerr << "zookeeper_init failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 设置调试参数
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    zoo_deterministic_conn_order(true);

    // 等待连接建立（带超时机制）
    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;  // 5秒超时
    if (sem_timedwait(&sem, &ts) == -1) {
        std::cerr << "Connect to ZooKeeper server timeout!" << std::endl;
        exit(EXIT_FAILURE);
    }
    sem_destroy(&sem);
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    char buffer[64];
    int bufferlen = sizeof(buffer);
    for (int i = 0; i < 3; ++i) {
        int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
        if (flag == ZOK)
            return buffer;
        if (flag == ZINVALIDSTATE)
            Reconnect();
    }
    return "";
}

//增强重连机制
void ZkClient::Reconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_zhandle) {
        zookeeper_close(m_zhandle);
        m_zhandle = nullptr;
    }

    // 指数退避重连
    int retries = 0;
    const int max_retries = 5;
    while (retries++ < max_retries) {
        try {
            Start();
            std::cout << "Reconnected successfully!" << std::endl;
            return;
        } catch (...) {
            int delay = 1 << retries;  // 指数退避
            std::this_thread::sleep_for(std::chrono::seconds(delay));
        }
    }
    std::cerr << "Max reconnect attempts reached!" << std::endl;
    exit(EXIT_FAILURE);
}
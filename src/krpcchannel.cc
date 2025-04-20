#include "krpcchannel.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <queue>
#include <string>
#include "krpcapplication.h"
#include "krpcheader.pb.h"
#include "zookeeperutil.h"

void KrpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                             google::protobuf::RpcController* controller,
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response,
                             google::protobuf::Closure* done) {
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();     // service_name
    std::string method_name = method->name();  // method_name

    // 获取参数的序列化字符串长度 args_size
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str)) {
        args_size = args_str.size();
    } else {
        controller->SetFailed("serialize request error!");
        return;
    }

    // 定义rpc的请求header
    krpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str)) {
        header_size = rpc_header_str.size();
    } else {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));  // header_size
    send_rpc_str += rpc_header_str;                               // rpcheader
    send_rpc_str += args_str;                                     // args

    // 使用连接池获取连接
    ZkClient& zkCli = ZkClient::GetInstance();
    zkCli.Start();  // 确保连接已建立
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data.empty()) {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }

    // 初始化连接池（假设每个服务对应一个连接池，这里简化处理）
    static std::unordered_map<std::string, KrpcConnectionPool*> poolMap;
    std::string poolKey = service_name + ":" + method_name;
    if (poolMap.find(poolKey) == poolMap.end()) {
        poolMap[poolKey] = KrpcConnectionPool::GetInstance();
        poolMap[poolKey]->Init(host_data);
    }
    KrpcConnectionPool* pool = poolMap[poolKey];

    int clientfd = pool->GetConnection();
    if (clientfd == -1) {
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "get connection from pool failed");
        controller->SetFailed(errtxt);
        return;
    }

    // 发送rpc请求
    if (send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1) {
        close(clientfd);  // 发送失败时关闭连接，不返回连接池
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 接收响应数据
    char recv_buf[1024] = {0};
    int recv_size = recv(clientfd, recv_buf, sizeof(recv_buf), 0);
    if (recv_size == -1) {
        close(clientfd);
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 反序列化响应
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        close(clientfd);
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    // 释放连接回连接池
    pool->ReleaseConnection(clientfd);
}
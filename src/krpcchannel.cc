#include "krpcchannel.h"
#include <string>
#include "krpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "krpcapplication.h"
#include "zookeeperutil.h"

/*
双方定义的数据类型：header_size + service_name method_name args_size + args
args表示proto定义的message中的变量
*/
void KrpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                google::protobuf::RpcController* controller, 
                                const google::protobuf::Message* request,
                                google::protobuf::Message* response,
                                google::protobuf::Closure* done)
{   
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name(); // service_name
    std::string method_name = method->name(); // method_name

    // 获取参数的序列化字符串长度 args_size
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
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
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4)); // header_size
    send_rpc_str += rpc_header_str; // rpcheader
    send_rpc_str += args_str; // args

    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "KrpcChannel::header_size: " << header_size << std::endl;
    std::cout << "KrpcChannel::rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "KrpcChannel::service_name: " << service_name << std::endl;
    std::cout << "KrpcChannel::method_name: " << method_name << std::endl;
    std::cout << "KrpcChannel::args_str: " << args_str.c_str() << std::endl;
    std::cout << "============================================" << std::endl;

    // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char errtxt[512] = {0};
        snprintf(errtxt, sizeof(errtxt), "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    
    /*
    //使用zookeeper前，需要读取配置文件rpcserver的信息
    std::string ip = KrpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(KrpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    */

    //rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();
    // 具体结点：/UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;
    // 127.0.0.1:8000
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str());
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 发送rpc请求
    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(clientfd);
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        close(clientfd);
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 反序列化rpc调用的响应数据 
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(clientfd);
        char errtxt[1024] = {0};
        snprintf(errtxt, sizeof(errtxt), "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }
    close(clientfd);

}
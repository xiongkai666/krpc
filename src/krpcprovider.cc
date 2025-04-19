#include "krpcapplication.h"
#include "krpcconfig.h"
#include <google/protobuf/descriptor.h>
#include <functional>
#include "krpcheader.pb.h"
#include "krpcprovider.h"

void KrpcProvider::NotifyService(google::protobuf::Service* service){
    ServiceInfo service_info;

    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();

    std::string service_name = pserviceDesc->name();
    std::cout << "service_name:" << service_name << std::endl;

    int methodCnt = pserviceDesc->method_count();
    
    for(int i = 0; i < methodCnt; ++i){
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        std::cout << "method_name:" << method_name << std::endl;

        service_info.m_methodMap.emplace(method_name, pmethodDesc);
    }
    service_info.m_service = service;
    m_serviceMap.emplace(service_name, service_info);
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void KrpcProvider::Run(){
    std::string ip = KrpcApplication::GetConfig().Load("rpcserverip");
    uint16_t port = atoi(KrpcApplication::GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    server.setConnectionCallback(std::bind(&KrpcProvider::OnConnection, this, std::placeholders::_1));

    server.setMessageCallback(std::bind(&KrpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    server.setThreadNum(4);

    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    server.start();
    m_eventLoop.loop();

}

void KrpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn) {
    if(!conn->connected()){
        conn->shutdown();
    }
}

/*
在框架内部，RpcProvider和RpcConsumer协商好通信用的protobuf数据类型，方便进行数据头的序列化和反序列化。
proto的message需要传输service_name method_name args。为了解决tcp粘包问题，定义的数据类型：header_size(4个字节) + header_str + args_str。
其中header_str记录：service_name method_name args_size。
*/
// 已建立连接用户的读写事件回调 如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
void KrpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp) {
    std::string recv_buf = buffer->retrieveAllAsString();

    uint32_t header_size = 0;
    //memcpy(&header_size, recv_buf.data(), 4);
    recv_buf.copy((char*)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    krpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str)) {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    } else {
        // 数据头反序列化失败
        std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "KrpcProvider::header_size: " << header_size << std::endl;
    std::cout << "KrpcProvider::rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "KrpcProvider::service_name: " << service_name << std::endl;
    std::cout << "KrpcProvider::method_name: " << method_name << std::endl;
    std::cout << "KrpcProvider::args_str: " << args_str << std::endl;
    std::cout << "============================================" << std::endl;

    // 获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end()) {
        std::cout << service_name << " is not exist!" << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end()) {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }

    google::protobuf::Service* service = it->second.m_service; 
    const google::protobuf::MethodDescriptor* method = mit->second;

    //生成rpc方法调用的请求request
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();

    // 生成rpc方法调用的响应response
    if (!request->ParseFromString(args_str)) {
        std::cout << "request parse error, content:" << args_str << std::endl;
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    // 当RPC调用完成后，会调用这个Closure对象，从而触发SendRpcResponse函数来处理RPC响应
    google::protobuf::Closure* done = google::protobuf::NewCallback<KrpcProvider,
    const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>(this,
    &KrpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    // 具体：new UserService().Login(controller, request, response, done)
    
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void KrpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response) {
    std::string response_str;
    if (response->SerializeToString(&response_str)) {
        // 序列化成功后，通过网络把rpc方法执行的结果发送会rpc的调用方
        conn->send(response_str);
    }
    else{
        std::cout << "serialize response_str error!" << std::endl; 
    }
    conn->shutdown(); // 模拟http的短链接服务，由rpcprovider主动断开连接
}
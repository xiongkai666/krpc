#include <iostream>
#include "krpcapplication.h"
#include "user.pb.h"
#include "krpcchannel.h"

int main(int argc, char **argv)
{
    // 首先调用框架的初始化函数（只初始化一次）
    KrpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login
    user::UserServiceRpc_Stub stub(new KrpcChannel());
    // rpc方法的请求参数
    user::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    // rpc方法的响应
    user::LoginResponse response;
    // 发起rpc方法的调用，统一调用RpcChannel::callmethod
    stub.Login(nullptr, &request, &response, nullptr);

    // 一次rpc调用完成，读调用的结果
    if (0 == response.result().errcode())
    {
        std::cout << "rpc login response success:" << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;
    }
    
    // 演示调用远程发布的rpc方法Register
    user::RegisterRequest req;
    req.set_id(2000);
    req.set_name("krpc");
    req.set_pwd("666666");
    user::RegisterResponse rsp;

    // 以同步的方式发起rpc调用请求，等待返回结果
    stub.Register(nullptr, &req, &rsp, nullptr);

    // 一次rpc调用完成，读调用的结果
    if (0 == rsp.result().errcode())
    {
        std::cout << "rpc register response success:" << rsp.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error : " << rsp.result().errmsg() << std::endl;
    }
    
    return 0;
}
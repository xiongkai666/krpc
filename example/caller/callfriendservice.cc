#include <iostream>
#include "friend.pb.h"
#include "krpcapplication.h"
#include "logger.h"

int main(int argc, char** argv) {

    //LOG_ERR("服务使用成功");
    //LOG_INFO("使用friend服务");

    // 先调用krpc框架的初始化函数（只初始化一次）
    KrpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login
    user::FiendServiceRpc_Stub stub(new KrpcChannel());
    // rpc方法的请求参数
    user::GetFriendsListRequest request;
    request.set_userid(1000);
    // rpc方法的响应
    user::GetFriendsListResponse response;
    // KrpcChannel->KrpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    
    KrpcController controller;
    stub.GetFriendsList(&controller, &request, &response, nullptr);

    // 一次rpc调用完成，读调用的结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            std::cout << "rpc GetFriendsList response success!" << std::endl;
            int size = response.friends_size();
            for (int i=0; i < size; ++i)
            {
                std::cout << "index:" << (i+1) << " name:" << response.friends(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc GetFriendsList response error : " << response.result().errmsg() << std::endl;
        }
    }

    return 0;
}
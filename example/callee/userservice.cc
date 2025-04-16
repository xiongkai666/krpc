#include<iostream>
#include<string>
#include"../user.pb.h"
#include"krpcapplication.h"
#include"krpcprovider.h"

class UserService: public user::UserServiceRpc{
public:
    bool Login(std::string name, std::string pwd){
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;
        return false;
    }
    void Login(::google::protobuf::RpcController* controller,
                       const ::user::LoginRequest* request,
                       ::user::LoginResponse* response,
                       ::google::protobuf::Closure* done){
        
        std::string name = request->name();
        std::string pwd = request->pwd();
        
        bool login_result = Login(name, pwd);

        user::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_sucess(login_result);

        done->Run();
    }
};

int main(int argc, char** argv){
    KrpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象，把UserService对象发布到rpc节点上
    KrpcProvider provider;
    provider.NotifyService(new UserService{});

    // 启动一个rpc服务发布节点。Run以后，进程进入阻塞状态，等待远程的rpc调用请求。
    provider.Run();

    return 0;
}
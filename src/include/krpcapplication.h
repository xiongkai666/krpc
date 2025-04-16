#pragma once

//krpc 框架的基础类,单例模式
class KrpcApplication{
public:
    static void Init(int argc, char ** argv);
    static KrpcApplication& GetInstance();
private:
    KrpcApplication(){}
    KrpcApplication(const KrpcApplication&) = delete;
    KrpcApplication(KrpcApplication &&) = delete;
};

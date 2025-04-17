#pragma once
#include "krpcconfig.h"

//krpc 框架的基础类,单例模式
class KrpcApplication{
public:
    static void Init(int argc, char ** argv);
    static KrpcApplication& GetInstance();
    static KrpcConfig& GetConfig();
private:
    KrpcApplication(){}
    KrpcApplication(const KrpcApplication&) = delete;
    KrpcApplication(KrpcApplication &&) = delete;
    static KrpcConfig m_config;

};

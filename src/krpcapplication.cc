#include "krpcapplication.h"
#include <unistd.h>
#include <iostream>

KrpcConfig KrpcApplication::m_config;

KrpcApplication& KrpcApplication::GetInstance() {
    static KrpcApplication app;
    return app;
}

void ShowArgsHelp() {
    std::cout << "format : commad -i <configfile>" << std::endl;
}

//初始化，读取配置文件
void KrpcApplication::Init(int argc, char** argv) {
    if(argc < 2){
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1) {
        switch (c) {
            case 'i':
                config_file = optarg;
                break;
            case '?':
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            case ':':
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    KrpcApplication::m_config.LoadConfigFile(config_file.c_str());
}



KrpcConfig& KrpcApplication::GetConfig(){
    return m_config;
}
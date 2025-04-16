#include"krpcapplication.h"

KrpcApplication& KrpcApplication::GetInstance() {
    static KrpcApplication app;
    return app;
}

void KrpcApplication::Init(int argc, char** argv){
    
}


#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include "zookeeperutil.h"
#include "krpcconnectionpool.h"

class KrpcChannel : public google::protobuf::RpcChannel {
public:
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) override;


};
#include "krpccontroller.h"

KrpcController::KrpcController() {
    m_failed = false;
    m_errText = "";
}

void KrpcController::Reset() {
    m_failed = false;
    m_errText = "";
}

bool KrpcController::Failed() const {
    return m_failed;
}

std::string KrpcController::ErrorText() const {
    return m_errText;
}

void KrpcController::SetFailed(const std::string& reason) {
    m_failed = true;
    m_errText = reason;
}

// 目前未实现具体的功能
void KrpcController::StartCancel() {}
bool KrpcController::IsCanceled() const {
    return false;
}
void KrpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}
#include "MyController.h"

MyController::MyController()
    : m_failed(false), m_errText("")
{
}

void MyController::Reset()
{
    m_failed = false;
    m_errText = "";
}

bool MyController::Failed() const
{
    return m_failed;
}

void MyController::SetFailed(const std::string& reason)
{
    m_failed = true;
    m_errText = reason;
}

std::string MyController::ErrorText() const
{
    return m_errText;
}

// ---------------- 下面这三个直接空实现即可 ----------------

void MyController::StartCancel()
{
    // 暂不提供取消 RPC 的功能
}

bool MyController::IsCanceled() const
{
    return false;
}

void MyController::NotifyOnCancel(google::protobuf::Closure* callback)
{
    // 暂不提供取消回调的功能
}
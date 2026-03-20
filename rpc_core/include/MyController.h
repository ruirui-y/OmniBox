#pragma once

#include <google/protobuf/service.h>
#include <string>

class MyController : public google::protobuf::RpcController
{
public:
    MyController();
    ~MyController() = default;

    // ---------------- 以下三个是核心功能，咱们必须实现 ----------------

    // 重置 Controller 状态（如果你想复用这个对象，就调一下它）
    void Reset() override;

    // 检查 RPC 调用是否失败（专门给调用端比如 Gateway 用的）
    bool Failed() const override;

    // 获取具体的错误信息（打印日志用）
    std::string ErrorText() const override;

    // 设置错误状态（专门给 MyChannel 底层用的）
    void SetFailed(const std::string& reason) override;

    // ---------------- 以下三个是高级/可选功能，暂时置空 ----------------

    // 尝试取消一个 RPC 调用（目前用不到）
    void StartCancel() override;

    // 检查 RPC 调用是否被取消
    bool IsCanceled() const override;

    // 注册一个回调，当 RPC 被取消时触发
    void NotifyOnCancel(google::protobuf::Closure* callback) override;

private:
    bool m_failed;          // 记录 RPC 方法执行过程中的状态
    std::string m_errText;  // 记录具体的错误信息
};
#pragma once
#include "meta_service.pb.h" 

#include <mymuduo/net/EventLoop.h>
#include <mymuduo/base/ThreadPool.h>
#include <mymuduo/db/DbExecutor.h>
using namespace omnibox;

class MetaServiceImpl : public omnibox::MetaService {
public:
    // 构造函数：注入主循环和线程池
    MetaServiceImpl(EventLoop* loop)
        : loop_(loop)
    {
        thread_pool_ = std::make_shared<ThreadPool>("Meta_ThreadPool");
        thread_pool_->start(4);
    }

    // 1. 新建目录
    virtual void CreateFolder(::google::protobuf::RpcController* controller,
        const ::omnibox::CreateFolderRequest* request,
        ::omnibox::CreateFolderResponse* response,
        ::google::protobuf::Closure* done) override;

    // 2. 删除节点 (软删除)
    virtual void DeleteNode(::google::protobuf::RpcController* controller,
        const ::omnibox::DeleteNodeRequest* request,
        ::omnibox::DeleteNodeResponse* response,
        ::google::protobuf::Closure* done) override;

    // 3. 秒传查岗
    virtual void CheckFile(::google::protobuf::RpcController* controller,
        const ::omnibox::CheckFileRequest* request,
        ::omnibox::CheckFileResponse* response,
        ::google::protobuf::Closure* done) override;

    // 4. 重命名节点
    virtual void RenameNode(::google::protobuf::RpcController* controller,
        const ::omnibox::RenameNodeRequest* request,
        ::omnibox::RenameNodeResponse* response,
        ::google::protobuf::Closure* done) override;

    // 5. 移动节点
    virtual void MoveNode(::google::protobuf::RpcController* controller,
        const ::omnibox::MoveNodeRequest* request,
        ::omnibox::MoveNodeResponse* response,
        ::google::protobuf::Closure* done) override;

    // 6. 拉取目录列表
    virtual void ListDirectory(::google::protobuf::RpcController* controller,
        const ::omnibox::ListDirectoryRequest* request,
        ::omnibox::ListDirectoryResponse* response,
        ::google::protobuf::Closure* done) override;

private:
    EventLoop* loop_;
    std::shared_ptr<ThreadPool> thread_pool_;
};
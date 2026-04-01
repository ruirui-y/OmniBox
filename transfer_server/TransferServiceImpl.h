#ifndef TRANSFER_SERVICE_IMPL_H
#define TRANSFER_SERVICE_IMPL_H

#include "transfer.pb.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <fstream>
#include <memory>

// 为每一个正在上传的文件，维护一个极其轻量的上下文
struct FileTransferContext
{
    std::mutex file_mutex;                                              // 专属这一个文件的锁，防止并发写同一个文件时磁头错乱
    std::fstream file_stream;                                           // 唯一的文件句柄
    std::atomic<int64_t> received_bytes{ 0 };                           // 极其忠诚的记账本：已收到多少字节
};

class TransferServiceImpl : public omnibox::TransferService
{
public:
	// 上传碎片
    void UploadChunk(::google::protobuf::RpcController* controller,
        const ::omnibox::FileChunkUploadRequest* request,
        ::omnibox::FileChunkUploadResponse* response,
        ::google::protobuf::Closure* done) override;

private:
    // 获取或创建文件的上下文
    std::shared_ptr<FileTransferContext> GetOrCreateContext(const std::string& file_name);

private:
    std::mutex map_mutex_;
    std::unordered_map<std::string, std::shared_ptr<FileTransferContext>> file_contexts_;
};

#endif
#include "TransferServiceImpl.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <mymuduo/Log/Logger.h>

void TransferServiceImpl::UploadChunk(::google::protobuf::RpcController* controller,
    const ::omnibox::FileChunkUploadRequest* request,
    ::omnibox::FileChunkUploadResponse* response,
    ::google::protobuf::Closure* done)
{
    std::string file_name = request->file_name();
    int64_t offset = request->offset();
    const std::string& data = request->data();
    bool is_eof = request->is_eof();

    // 1. 获取文件的专属上下文
    auto ctx = GetOrCreateContext(file_name);

    // 2. 独占这个文件
    {
        std::lock_guard<std::mutex> file_lock(ctx->file_mutex);                                 // 锁住这一个文件, 但是只有一个事件循环，加不加锁意义不大
        if (ctx->file_stream.is_open())
        {
            ctx->file_stream.seekp(offset, std::ios::beg);
            ctx->file_stream.write(data.c_str(), data.size());
        }
    }

    // 3. 忠诚记账：不管你是第几块，落地了就把体积加上去
    int64_t current_total = ctx->received_bytes.fetch_add(data.size()) + data.size();

    if (current_total == request->total_size())
    {
        LOG_INFO << "====== 传输大满贯！====== 字节分毫不差，彻底落盘！";

        // 收尾工作：关掉文件，把这个上下文从大字典里踢出去
        ctx->file_stream.close();
        std::lock_guard<std::mutex> lock(map_mutex_);
        file_contexts_.erase(file_name);
    }
    
    // 通知网关
    response->set_success(true);
    response->set_message("Chunk writeen successfully");
    response->set_next_offset(offset + data.size());

    if (done) 
    {
        done->Run();
    }
}

std::shared_ptr<FileTransferContext> TransferServiceImpl::GetOrCreateContext(const std::string& file_name)
{
    std::lock_guard<std::mutex> lock(map_mutex_);
    if (file_contexts_.find(file_name) == file_contexts_.end())
    {
        auto ctx = std::make_shared<FileTransferContext>();
        // 注意：这里绝不能用 trunc！用 out | binary，如果文件不存在会自动创建空洞文件
        ctx->file_stream.open("./" + file_name, std::ios::binary | std::ios::out);
        file_contexts_[file_name] = ctx;
    }
    return file_contexts_[file_name];
}
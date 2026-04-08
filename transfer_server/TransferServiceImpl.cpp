#include "TransferServiceImpl.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <mymuduo/Log/Logger.h>

TransferServiceImpl::TransferServiceImpl()
    : disk_io_pool_("DiskIOPool")
{
    disk_io_pool_.start(4); // 启动 4 个专门写硬盘的线程
}

void TransferServiceImpl::UploadChunk(::google::protobuf::RpcController* controller,
    const ::omnibox::FileChunkUploadRequest* request,
    ::omnibox::FileChunkUploadResponse* response,
    ::google::protobuf::Closure* done)
{
    // 1. 提取必要的参数（在主网络线程中完成，极速操作）
        // ⚠️ 极其关键：把 string 数据按值拷贝出来！
        // 因为这层函数马上就会 return，为了防止多线程下底层 Protobuf 对象的生命周期出幺蛾子，
        // 我们把最核心的 data 拷贝一份放到 Lambda 里带走，彻底解耦，绝对安全！
    std::string file_name = request->file_name();
    int64_t offset = request->offset();
    std::string data = request->data();
    int64_t total_size = request->total_size();

    // 2. 把沉重的磁盘 I/O 像炸药包一样，直接丢进专属的后台线程池！
    // 注意捕获列表：通过值捕获(拷贝)参数，通过指针捕获 response 和 done
    disk_io_pool_.run([this, file_name, offset, data, total_size, response, done]() 
        {
            // 1. 获取文件的专属上下文
            auto ctx = GetOrCreateContext(file_name);
            // 2. 独占这个文件
            {
                // 🚨 架构师护盾：现在有 4 个后台线程，这把锁极其关键，防并发踩踏全靠它！
                std::lock_guard<std::mutex> file_lock(ctx->file_mutex);
                if (ctx->file_stream.is_open())
                {
                    ctx->file_stream.seekp(offset, std::ios::beg);
                    ctx->file_stream.write(data.c_str(), data.size());
                }
            } // 锁在这里释放，其他线程立刻可以接着写下一块

            // 3. 忠诚记账：不管你是第几块，落地了就把体积加上去
            int64_t current_total = ctx->received_bytes.fetch_add(data.size()) + data.size();

            if (current_total == total_size)
            {
                LOG_INFO << "====== 传输大满贯！====== 字节分毫不差，彻底落盘！";

                // 收尾工作：关掉文件，把这个上下文从大字典里踢出去
                ctx->file_stream.close();
                std::lock_guard<std::mutex> lock(map_mutex_);
                file_contexts_.erase(file_name);
            }

            // 4. 组装网关响应
            response->set_success(true);
            response->set_message("Chunk writeen successfully");
            response->set_next_offset(offset + data.size());

            // 5. 👑 终极控制：在这里，且只有在这里，才能调用闭包！
            // 只要硬盘没写完，这句话就不会执行，网关就不会给前端发 HTTP 200，前端的 Promise 就会死死卡住等待！
            if (done)
            {
                done->Run();
            }
        });
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
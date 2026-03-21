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

    // 先存在当前目录下
    std::string save_path = "./" + file_name;

    // 如果是第一块，用trunc模式清空/新建文件
    // 如果不是，用in | out 模式打开，配和seekp 进行偏移量覆盖写
    std::ios_base::openmode mode = std::ios::binary | std::ios::out;
    mode |= offset == 0 ? std::ios::trunc : std::ios::in;

    std::fstream file(save_path, mode);

    // 如果文件之前不存在且不是第一块
    if (!file.is_open())
    {
        file.open(save_path, std::ios::binary | std::ios::out | std::ios::trunc);
    }

    if (file.is_open())
    {
        // 将磁盘磁头移动到网关指定的offset位置
        file.seekp(offset, std::ios::beg);

        // 写入这块数据碎片
        file.write(data.c_str(), data.size());
        file.close();

        // 通知网关
        response->set_success(true);
        response->set_message("Chunk writeen successfully");
        response->set_next_offset(offset + data.size());

        if (is_eof)
        {
            LOG_INFO << "====== 传输大满贯！====== 文件[" << file_name << "] 碎片组装完毕，彻底落盘！";
        }
    }
    else
    {
        response->set_success(false);
        response->set_message("TransferServer failed to open file for writing");
    }

    if (done) 
    {
        done->Run();
    }
}
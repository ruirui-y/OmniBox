#include "RPCServer.h"
#include "rpcheader.pb.h"
#include "RPCClosure.h"
#include <mymuduo/Log/Logger.h>
#include <endian.h>

void RPCServer::RegisterService(google::protobuf::Service* service)
{
	services[service->GetDescriptor()->name()] = service;
}

void RPCServer::Run()
{
	TcpServer server(&event_loop_, ip_, port_, 1000);
	server.SetMessageCallback(
		[this](const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
		{
			OnMessage(conn, buffer);
		});

	server.Start(0);
	event_loop_.Loop();
}

void RPCServer::OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
{
    while (buffer->ReadableBytes() >= 4)
    {
        // 1. 偷看前4个字节（不从buffer剔除），获取 header_size
        uint32_t header_size = buffer->PeekInt32();

        // 2. 检查：连 rpc_header 都还没收全？
        if (buffer->ReadableBytes() < 4 + header_size) 
        {
            break;                                                                                  // 半包，跳出循环等下一次 epoll 唤醒
        }

        // 3. 偷拿 header 数据进行反序列化（依然不剔除 buffer，因为 args 还没确认收全）
        std::string rpc_header_str(buffer->peek() + 4, header_size);
        rpc::core::RpcHeader rpcHeader;
        if (!rpcHeader.ParseFromString(rpc_header_str)) {
            LOG_ERROR << "rpc header parse error!";
            return;
        }

        uint32_t args_size = rpcHeader.args_size();

        // 4. 检查：真正的请求体 (args) 收全了吗？
        if (buffer->ReadableBytes() < 4 + header_size + args_size) 
        {
            break;                                                                                  // 半包，跳出循环继续等
        }

        // ============ 走到这里，说明一个完整的 RPC 请求终于拼齐了！============

        // 5. 正式把这个完整包从 Buffer 里剥离出来
        buffer->retrieve(4);                                                                        // 剥离长度信息
        buffer->retrieve(header_size);                                                              // 剥离 header
        std::string args_str = buffer->RetrieveAsString(args_size);                                 // 剥离并获取真正的参数数据

        // 6. 后续的业务分发逻辑 (找服务、生成 request、CallMethod)
        std::string service_name = rpcHeader.service_name();
        auto it = services.find(service_name);
        if (it == services.end()) {
            LOG_ERROR << service_name << " is not exist!";
            return;
        }

        google::protobuf::Service* service = it->second;
        const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->method(rpcHeader.method_index());

        google::protobuf::Message* request = service->GetRequestPrototype(method).New();
        if (!request->ParseFromString(args_str)) 
        {
            LOG_ERROR << "request parse error!";
            delete request;                                                                         // 防止解析失败时内存泄漏
            return;
        }
        google::protobuf::Message* response = service->GetResponsePrototype(method).New();

        uint64_t seq_id = rpcHeader.seq_id();

        google::protobuf::Closure* done = new RPCClosure(
            [this, conn, response, seq_id]() {
                this->SendRpcResponse(conn, response, seq_id);
            }
        );
        service->CallMethod(method, nullptr, request, response, done);

        // 释放堆空间
        if (request) delete request;
        if (response) delete response;
    }
}

// 长度(4B) + 序列号(8B) + 内容
void RPCServer::SendRpcResponse(const std::shared_ptr<TcpConnection>& conn, google::protobuf::Message* response, uint64_t seq_id)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {
        // 1. 计算除了 4 字节长度头之外的“总长度”：8字节 SeqID + PB数据长度
        uint32_t total_len = 8 + response_str.size();

        // 转换长度为网络字节序
        uint32_t net_len = htonl(total_len);

        // 2. 将 64 位单号转换为网络字节序 (Host to Big Endian 64)
        uint64_t net_seq_id = htobe64(seq_id);

        // 3. 完美拼装二进制流 (使用 append 更安全高效，避免 string 的 + 操作符可能带来的 '\0' 截断风险)
        std::string send_str;
        send_str.append((char*)&net_len, 4);                                                            // 头部 4 字节
        send_str.append((char*)&net_seq_id, 8);                                                         // 单号 8 字节
        send_str.append(response_str);                                                                  // 真实 PB 响应

        // 发送给网关
        conn->Send(send_str);
    }
    else
    {
        LOG_ERROR << "serialize response_str error!";
    }
}
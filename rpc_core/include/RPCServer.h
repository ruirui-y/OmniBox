#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <mymuduo/net/TcpServer.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

class RPCServer 
{
public:
	RPCServer(std::string ip, uint16_t port)
		:ip_(ip), port_(port) {}

	void RegisterService(google::protobuf::Service* service);
	void Run();

private:
	void OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer);
	void SendRpcResponse(const std::shared_ptr<TcpConnection>& conn, google::protobuf::Message* response, uint64_t seq_id);

private:
	std::unordered_map<std::string, google::protobuf::Service*> services;
	EventLoop event_loop_;
	std::string ip_;
	uint16_t port_;
};

#endif // RPC_SERVER_H
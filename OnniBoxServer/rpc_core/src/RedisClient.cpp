#include "RedisClient.h"
#include <mymuduo/Log/Logger.h>

RedisClient::RedisClient()
	:context_(nullptr)
{
}

RedisClient::~RedisClient()
{
	Disconnect();
}

// 连接与断开
bool RedisClient::Connect(const std::string& host, int port)
{
	context_ = redisConnect(host.c_str(), port);
	if (context_ == nullptr || context_->err)
	{
		if (context_)
		{
			LOG_INFO << "[RedisClient] 连接错误: " << context_->errstr;
			redisFree(context_);
			context_ = nullptr;
		}
		else
		{
			LOG_INFO << "[RedisClient] 分配 Redis 上下文失败";
		}
		return false;
	}

	// LOG_INFO << "[RedisClient] 成功连接到 Redis 服务器 " << host << ":" << port;
	return true;
}

void RedisClient::Disconnect()
{
	if(context_)
	{
		redisFree(context_);
		context_ = nullptr;
	}
}

// key-value操作
bool RedisClient::Set(const std::string& key, const std::string& value)
{
	if (!context_) return false;
	redisReply* reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
	if (reply == nullptr) return false;

	bool success = !(reply->type == REDIS_REPLY_ERROR);
	freeReplyObject(reply);
	return success;
}

std::string RedisClient::Get(const std::string& key, std::string* value)
{
	if (!context_) return "";
	redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
	if (reply == nullptr) return "";

	std::string result = reply->type == REDIS_REPLY_STRING ? result = reply->str : "";
	freeReplyObject(reply);
	return result;
}

bool RedisClient::Del(const std::string& key)
{
	if (!context_) return false;
	redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
	if (reply == nullptr) return false;

	freeReplyObject(reply);
	return true;
}

// Hash操作
bool RedisClient::HSet(const std::string& key, const std::string& field, const std::string& value)
{
	if (!context_) return false;
	redisReply* reply = (redisReply*)redisCommand(context_, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
	if (reply == nullptr) return false;

	bool success = !(reply->type == REDIS_REPLY_ERROR);
	freeReplyObject(reply);
	return success;
}

std::string RedisClient::HGet(const std::string& key, const std::string& field)
{
	if (!context_) return "";
	redisReply* reply = (redisReply*)redisCommand(context_, "HGET %s %s", key.c_str(), field.c_str());
	if (reply == nullptr) return "";

	std::string result = reply->type == REDIS_REPLY_STRING ? reply->str : "";
	freeReplyObject(reply);
	return result;
}

// 原子操作：如果Key不存在则设置并返回true(抢锁成功)，存在则返回false(抢锁失败)
// 必须带有过期时间，防止拿到锁的微服务突然崩溃导致死锁
bool RedisClient::SetNx(const std::string& key, const std::string& value, int expire_seconds)
{
	if (!context_) return false;
	// SET key value EX seconds NX 
	// 这是 Redis 官方推荐的实现分布式锁的最安全写法
	redisReply* reply = (redisReply*)redisCommand(context_, "SET %s %s EX %d NX",
		key.c_str(), value.c_str(), expire_seconds);
	if (reply == nullptr) return false;

	// 如果抢锁成功，Redis 会返回 "OK" (状态回复)
	// 如果 Key 已经存在（被人抢了），Redis 会返回 NIL 回复
	bool success = (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");

	freeReplyObject(reply);
	return success;
}
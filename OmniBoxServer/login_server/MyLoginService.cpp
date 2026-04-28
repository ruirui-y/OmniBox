#include "MyLoginService.h"

#include <mymuduo/Log/Logger.h>
#include <mymuduo/db/DbExecutor.h>
#include <mymuduo/base/ThreadPool.h>
#include <mymuduo/net/EventLoop.h> // 确保引入了 EventLoop 的完整定义

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

using namespace game::rpc;

// 构造函数
MyLoginService::MyLoginService(EventLoop* loop, std::shared_ptr<ThreadPool> threadPool)
    : loop_(loop), thread_pool_(threadPool)
{
}

// 登录接口实现
void MyLoginService::Login(::google::protobuf::RpcController* controller,
    const ::game::rpc::LoginRequest* request,
    ::game::rpc::LoginResponse* response,
    ::google::protobuf::Closure* done)
{
    // 1. 框架给业务吐出请求参数
    std::string name = request->username();
    std::string pwd = request->password();

    // 2. 构造查询语句 
    std::string sql = "SELECT user_id, password, status, pwd_version, "
        "IF(last_heartbeat_time > (NOW() - INTERVAL 30 SECOND), 1, 0) AS is_active "
        "FROM user_info WHERE username = ?";
    DbParams params = { name };

    // 3. 异步查库
    DbExecutor::AsyncQuery(loop_, thread_pool_.get(), sql, params,
        [this, name, pwd, response, done](const DbResultSet& results)
        {
            if (!results.empty())
            {
                const auto& row = results[0];

                int status = std::get<int>(row.at("status"));
                std::string db_pwd = std::get<std::string>(row.at("password"));
                int64_t uid = std::get<int64_t>(row.at("user_id"));
                int pwd_version = std::get<int>(row.at("pwd_version"));

                // 获取动态计算出的在线状态
                bool bonline = std::get<int64_t>(row.at("is_active")) == 1;

                if (status == 0) {
                    response->set_errcode(403);
                    response->set_errmsg("Account has been banned");
                }
                else if (bonline)
                {
                    response->set_errcode(403);
                    response->set_errmsg("User already online. Multiple login is forbidden.");
                }
                else
                {
                    bool is_match = false;

                    // --- 核心逻辑：版本分支处理 ---
                    if (pwd_version == 0)
                    {
                        // A. 历史遗留明文：直接比对
                        if (pwd == db_pwd)
                        {
                            is_match = true;
                            // 触发异步静默升级
                            LOG_INFO << "[Auth Upgrade] User " << name << " using plain text, migrating to SHA256...";

                            std::string new_hash = sha256(pwd);
                            std::string update_sql = "UPDATE user_info SET password = ?, pwd_version = 1, last_heartbeat_time = NOW(), last_login_time = NOW() WHERE user_id = ?";
                            DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), update_sql, { new_hash, uid }, nullptr);
                        }
                    }
                    else
                    {
                        // B. 新版哈希：哈希后再比对
                        if (sha256(pwd) == db_pwd)
                        {
                            is_match = true;
                            std::string update_sql = "UPDATE user_info SET last_heartbeat_time = NOW(), last_login_time = NOW() WHERE user_id = ?";
                            DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), update_sql, { uid }, nullptr);
                        }
                    }

                    if (is_match)
                    {
                        response->set_errcode(0);
                        response->set_errmsg("Login Success");
                        response->set_token("11111111");
                        response->set_user_id(uid);
                        LOG_INFO << "[Login Success] username = " << name << ", uid = " << uid;
                    }
                    else
                    {
                        response->set_errcode(401);
                        response->set_errmsg("Invalid password");
                    }
                }
            }
            else
            {
                response->set_errcode(404);
                response->set_errmsg("User not found");
            }

            if (done) {
                done->Run();
            }
        });
}

// 心跳接口实现
void MyLoginService::Heartbeat(::google::protobuf::RpcController* controller,
    const ::game::rpc::HeartbeatRequest* request,
    ::game::rpc::HeartbeatResponse* response,
    ::google::protobuf::Closure* done)
{
    int64_t uid = request->user_id();
    LOG_INFO << "111111111111";
    std::string sql = "UPDATE user_info SET last_heartbeat_time = NOW() WHERE user_id = ?";

    DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), sql, { uid },
        [response, done](int affectedRows, int64_t /*insertId*/) {
            if (affectedRows > 0) {
                response->set_success(true);
                response->set_server_time(time(NULL));
            }
            else {
                response->set_success(false);
            }

            if (done) {
                done->Run();
            }
        });
}

// SHA256 加密实现
std::string MyLoginService::sha256(const std::string& str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
#include "MyLoginService.h"

#include <mymuduo/Log/Logger.h>
#include <mymuduo/db/DbExecutor.h>
#include <mymuduo/base/ThreadPool.h>
#include <mymuduo/net/EventLoop.h> // 确保引入了 EventLoop 的完整定义

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include "common.pb.h"


// 构造函数
MyLoginService::MyLoginService(EventLoop* loop, std::shared_ptr<ThreadPool> threadPool)
    : loop_(loop), thread_pool_(threadPool)
{
}

// 登录接口实现
void MyLoginService::Login(::google::protobuf::RpcController* controller,
    const ::LoginRequest* request,
    ::LoginResponse* response,
    ::google::protobuf::Closure* done)
{
    // 1. 框架给业务吐出请求参数
    std::string name = request->username();
    std::string pwd = request->password();

    // 2. 构造查询语句 
    // 移除了对 is_active 的查询，因为在线校验已经彻底下放给了网关，减轻 MySQL 压力
    std::string sql = "SELECT user_id, password, status, pwd_version FROM user_info WHERE username = ?";
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

                // 判断账号是否被封禁
                if (status == 0) {
                    response->set_errcode(ERR_ACCOUNT_BANNED);
                    response->set_errmsg("Account has been banned");
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
                        response->set_errcode(omnibox::ERR_SUCCESS);
                        response->set_errmsg("Login Success");
                        response->set_token("11111111"); // TODO: 后续可接入真实的 JWT 或随机 Token 生成
                        response->set_user_id(uid);
                        LOG_INFO << "[Login Success] username = " << name << ", uid = " << uid;
                    }
                    else
                    {
                        response->set_errcode(omnibox::ERR_WRONG_PWD);
                        response->set_errmsg("Invalid password");
                    }
                }
            }
            else
            {
                // 账号不存在
                response->set_errcode(omnibox::ERR_USER_NOT_FOUND);
                response->set_errmsg("User not found");
            }

            // 4. 执行回调闭包，将组装好的 response 传回给发起调用的网关
            if (done) {
                done->Run();
            }
        });
}

// 心跳接口实现
void MyLoginService::Heartbeat(::google::protobuf::RpcController* controller,
    const ::HeartbeatRequest* request,
    ::HeartbeatResponse* response,
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
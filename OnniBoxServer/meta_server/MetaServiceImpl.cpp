#include "MetaServiceImpl.h"
#include <muduo/base/Logging.h>

// ==========================================================
// 1. 新建目录
// ==========================================================
void MetaServiceImpl::CreateFolder(::google::protobuf::RpcController* controller,
    const ::omnibox::CreateFolderRequest* request,
    ::omnibox::CreateFolderResponse* response,
    ::google::protobuf::Closure* done)
{
    std::string sql = "INSERT INTO virtual_file_node (user_id, parent_id, node_name, is_dir, status) VALUES (?, ?, ?, 1, 1)";
    DbParams params = { request->user_id(), request->parent_id(), request->folder_name() };

    DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), sql, params, [response, done](int affectedRows, int lastInsertId)
        {
            if (affectedRows > 0) 
            {
                response->set_success(true);
                response->set_message("目录创建成功");
                response->set_new_node_id(lastInsertId);
            }
            else 
            {
                response->set_success(false);
                response->set_message("目录创建失败，可能存在同名目录");
            }

            if (done)
            {
                done->Run(); // 👑 唤醒 RPC 框架
            }
        }, true);
}

// ==========================================================
// 2. 删除节点 (防手滑：软删除机制)
// ==========================================================
void MetaServiceImpl::DeleteNode(::google::protobuf::RpcController* controller,
    const ::omnibox::DeleteNodeRequest* request,
    ::omnibox::DeleteNodeResponse* response,
    ::google::protobuf::Closure* done)
{
    // status = 2 代表进入回收站
    std::string sql = "UPDATE virtual_file_node SET status = 2, update_time = NOW() WHERE node_id = ? AND user_id = ?";
    DbParams params = { request->node_id(), request->user_id() };

    DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), sql, params, [response, done](int affectedRows, int lastInsertId) 
        {
            response->set_success(affectedRows > 0);
            response->set_message(affectedRows > 0 ? "已移至回收站" : "删除失败或越权操作");
            if (done)
            {
                done->Run();
            }
        });
}

// ==========================================================
// 3. 秒传查岗 (嵌套异步：先查后插)
// ==========================================================
void MetaServiceImpl::CheckFile(::google::protobuf::RpcController* controller,
    const ::omnibox::CheckFileRequest* request,
    ::omnibox::CheckFileResponse* response,
    ::google::protobuf::Closure* done)
{
    std::string query_sql = "SELECT 1 FROM virtual_file_node WHERE file_hash = ? AND status = 1 LIMIT 1";
    DbParams query_params = { request->file_hash() };

    DbExecutor::AsyncQuery(loop_, thread_pool_.get(), query_sql, query_params,
        [this, request, response, done](const DbResultSet& results) 
        {
            if (!results.empty()) 
            {
                // 命中秒传！执行幻象建房
                std::string insert_sql = "INSERT INTO virtual_file_node (user_id, parent_id, node_name, is_dir, file_hash, file_size, status) VALUES (?, ?, ?, 0, ?, ?, 1)";
                DbParams insert_params = { request->user_id(), request->parent_id(), request->file_name(), request->file_hash(), request->file_size() };

                DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), insert_sql, insert_params,
                    [response, done](int affectedRows, int lastInsertId) {
                        response->set_is_exist(true);
                        response->set_message("秒传挂载成功");
                        if (done)
                        {
                            done->Run();
                        }
                    });
            }
            else {
                // 未命中
                response->set_is_exist(false);
                response->set_message("文件未找到，请开始上传");
                if (done)
                {
                    done->Run();
                }
            }
        });
}

// ==========================================================
// 4. 重命名节点
// ==========================================================
void MetaServiceImpl::RenameNode(::google::protobuf::RpcController* controller,
    const ::omnibox::RenameNodeRequest* request,
    ::omnibox::RenameNodeResponse* response,
    ::google::protobuf::Closure* done)
{
    std::string sql = "UPDATE virtual_file_node SET node_name = ?, update_time = NOW() WHERE node_id = ? AND user_id = ?";
    DbParams params = { request->new_name(), request->node_id(), request->user_id() };

    DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), sql, params, [response, done](int affectedRows, int lastInsertId)
        {
            response->set_success(affectedRows > 0);
            response->set_message(affectedRows > 0 ? "重命名成功" : "重命名失败，名称冲突或无权限");
            if (done)
            {
                done->Run();
            }
        });
}

// ==========================================================
// 5. 移动节点
// ==========================================================
void MetaServiceImpl::MoveNode(::google::protobuf::RpcController* controller,
    const ::omnibox::MoveNodeRequest* request,
    ::omnibox::MoveNodeResponse* response,
    ::google::protobuf::Closure* done)
{
    std::string sql = "UPDATE virtual_file_node SET parent_id = ?, update_time = NOW() WHERE node_id = ? AND user_id = ?";
    DbParams params = { request->target_parent_id(), request->node_id(), request->user_id() };

    DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), sql, params, [response, done](int affectedRows, int lastInsertId) 
        {
            response->set_success(affectedRows > 0);
            response->set_message(affectedRows > 0 ? "移动成功" : "移动失败");
            if (done)
            {
                done->Run();
            }
        });
}

// ==========================================================
// 6. 拉取目录列表 (极其核心，负责前台展示)
// ==========================================================
void MetaServiceImpl::ListDirectory(::google::protobuf::RpcController* controller,
    const ::omnibox::ListDirectoryRequest* request,
    ::omnibox::ListDirectoryResponse* response,
    ::google::protobuf::Closure* done)
{
    // 注意：用 DATE_FORMAT 把 datetime 转成字符串，方便 C++ 接收
    std::string sql =
        "SELECT node_id, node_name, is_dir, file_size, file_hash, DATE_FORMAT(update_time, '%Y-%m-%d %H:%i:%s') as update_time "
        "FROM virtual_file_node "
        "WHERE user_id = ? AND parent_id = ? AND status = 1 "
        "ORDER BY is_dir DESC, update_time DESC";

    DbParams params = { request->user_id(), request->parent_id() };

    DbExecutor::AsyncQuery(loop_, thread_pool_.get(), sql, params, [response, done](const DbResultSet& results)
        {
            response->set_success(true);
            response->set_message("获取列表成功");

            // 遍历数据库结果，塞入 Protobuf 的 repeated 字段中
            for (const auto& row : results) {
                auto* node = response->add_nodes();

                // 👑 修复：使用列名字符串作为 Key 取值！
                node->set_node_id(std::get<int64_t>(row.at("node_id")));
                node->set_node_name(std::get<std::string>(row.at("node_name")));

                // is_dir 在数据库里可能是 TINYINT，你底层 DbExecutor 提取成了 int
                node->set_is_dir(std::get<int>(row.at("is_dir")) == 1);

                node->set_file_size(std::get<int64_t>(row.at("file_size")));

                // file_hash 可能是 NULL (比如文件夹)，处理一下
                if (std::holds_alternative<std::string>(row.at("file_hash"))) {
                    node->set_file_hash(std::get<std::string>(row.at("file_hash")));
                }
                else {
                    node->set_file_hash("");
                }

                // update_time 已经被我们在 SQL 里用 DATE_FORMAT 转成了字符串
                node->set_update_time(std::get<std::string>(row.at("update_time")));
            }

            if (done)
            {
                done->Run();
            }
        });
}
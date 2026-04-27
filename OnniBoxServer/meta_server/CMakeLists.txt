# ==========================================
# 元数据服务节点
# ==========================================

# 搜集当前目录下的源码
file(GLOB META_SRCS "*.cpp")

# 生成可执行文件
add_executable(meta_server ${META_SRCS})

# 链接 rpc_core 库
target_link_libraries(meta_server rpc_core)
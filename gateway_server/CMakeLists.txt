# ==========================================
# 网关服务节点
# ==========================================

# 搜集当前目录下的源码
file(GLOB GATEWAY_SRCS "*.cpp")

# 生成可执行文件
add_executable(gateway_server ${GATEWAY_SRCS})

# 链接 rpc_core 库
target_link_libraries(gateway_server rpc_core)
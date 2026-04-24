# ==========================================
# 登录服务节点
# ==========================================

# 搜集当前目录下的源码
file(GLOB LOGIN_SRCS "*.cpp")

# 生成可执行文件
add_executable(login_server ${LOGIN_SRCS})

# 寻找系统中的 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 链接你刚刚写的 rpc_core 库
target_link_libraries(login_server PRIVATE rpc_core OpenSSL::Crypto)
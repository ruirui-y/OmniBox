cmake_minimum_required(VERSION 3.10)
project(GameRpcProject)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 全局查找 Protobuf
find_package(Protobuf REQUIRED)

# 设置全局头文件搜索路径 (让所有子目录都能找到 pb 和 rpc_core 的头文件)
include_directories(
    ${PROJECT_SOURCE_DIR}/rpc_core/include
    ${PROJECT_SOURCE_DIR}/pb
    ${Protobuf_INCLUDE_DIRS}
    /usr/local/include/mymuduo
)

# 依次添加子目录 (注意顺序：先编译底层的 rpc_core，再编译上层服务)
add_subdirectory(rpc_core)
add_subdirectory(login_server)
add_subdirectory(gateway_server)
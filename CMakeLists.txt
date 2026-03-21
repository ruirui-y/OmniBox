cmake_minimum_required(VERSION 3.10)
project(OmniBox)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================================
# 2. 【核心架构调整】统一可执行文件的输出路径
# 无论子目录嵌套多深，所有编译出的可执行文件，全部统一打到 out/bin 目录下
# =========================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# =========================================================================
# 3. 【后勤补给线】将前端阵地自动同步到战区
# 每次构建时，自动把 src/www 文件夹完整拷贝到 out/bin 目录下
# 这样你的网关一启动，./www 就在它眼皮子底下！
# =========================================================================
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/www DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

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
add_subdirectory(transfer_server)
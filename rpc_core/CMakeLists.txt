# ==========================================
# 专门负责 rpc_core 静态库的编译和 proto 生成
# ==========================================

set(PROTO_SRC_DIR ${CMAKE_SOURCE_DIR}/proto)
set(PB_OUT_DIR ${CMAKE_SOURCE_DIR}/pb)

# 1. 找到所有的 .proto 文件
file(GLOB PROTO_FILES "${PROTO_SRC_DIR}/*.proto")

set(PROTO_PCH_SRCS "")
set(PROTO_PCH_HDRS "")

# 2. 遍历每一个 proto 文件，单独为它建立编译规则
foreach(PROTO_FILE ${PROTO_FILES})
    # 提取纯文件名 (如 login)
    get_filename_component(FIL_WE ${PROTO_FILE} NAME_WE)
    
    # 定义这个 proto 专属的输出产物
    set(OUT_CC "${PB_OUT_DIR}/${FIL_WE}.pb.cc")
    set(OUT_H  "${PB_OUT_DIR}/${FIL_WE}.pb.h")
    
    # 【核心修复】：为每一个 proto 文件单独绑定一条 protoc 命令！
    # 只要当前的 ${PROTO_FILE} 有改动，就一定会触发重新生成它的 OUT_CC 和 OUT_H
    add_custom_command(
        OUTPUT ${OUT_CC} ${OUT_H}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out=${PB_OUT_DIR} -I${PROTO_SRC_DIR} ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "正在编译协议: ${FIL_WE}.proto ..."
    )
    
    # 收集生成的文件名供打静态库使用
    list(APPEND PROTO_PCH_SRCS ${OUT_CC})
    list(APPEND PROTO_PCH_HDRS ${OUT_H})
endforeach()

# 4. 获取源码
file(GLOB RPC_CORE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 5. 生成静态库 rpc_core
add_library(rpc_core STATIC ${RPC_CORE_SRCS} ${PROTO_PCH_SRCS})

# 6. 链接依赖
target_link_libraries(rpc_core PUBLIC ${Protobuf_LIBRARIES} pthread mymuduo mysqlcppconn hiredis zookeeper_mt)
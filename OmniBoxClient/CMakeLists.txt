# ==============================================================================
# 项目名称: OmniBoxClient Client
# 核心架构: Qt5 + Protobuf + CMake
# ==============================================================================

cmake_minimum_required(VERSION 3.16)
project(OmniBoxClient VERSION 1.0 LANGUAGES CXX)

# ------------------------------------------------------------------------------
# 1. 编译标准与全局属性
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 元对象系统的自动化处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ------------------------------------------------------------------------------
# 2. 外部依赖 (Dependencies)
# ------------------------------------------------------------------------------
# 兜底注入 Vcpkg 路径，防止被 Qt 环境变量覆盖
list(APPEND CMAKE_PREFIX_PATH "D:/YJJ/vcpkg/installed/x64-windows")

# 引入 Qt5 核心与多媒体组件
find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    Multimedia 
    MultimediaWidgets 
    REQUIRED
)

# 引入 Protobuf 序列化库
find_package(Protobuf CONFIG REQUIRED)

# ------------------------------------------------------------------------------
# 3. 源码管理 (Source Files)
# ------------------------------------------------------------------------------
# 自动抓取 src 下的所有 C++, Protobuf(.cc) 和 UI 文件
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
    "src/*.cpp"
    "src/*.cc"
    "src/*.h"
    "src/*.ui"
    "src/demand.qrc"
)

# ------------------------------------------------------------------------------
# 4. 构建目标 (Build Target)
# ------------------------------------------------------------------------------
# WIN32 标志用于在 Windows 下隐藏控制台黑框
add_executable(OmniBoxClient WIN32 ${SRC_FILES}) 

# ------------------------------------------------------------------------------
# 5. 链接与包含路径配置 (Link & Include)
# ------------------------------------------------------------------------------
# 链接第三方库
target_link_libraries(OmniBoxClient PRIVATE 
    Qt5::Core 
    Qt5::Gui 
    Qt5::Widgets 
    Qt5::Network
    Qt5::Multimedia         
    Qt5::MultimediaWidgets  
    protobuf::libprotobuf
)

# --- 动态生成扁平化头文件包含目录 ---
set(ALL_INCLUDE_DIRS "")
foreach(FILE_PATH ${SRC_FILES})
    get_filename_component(DIR_PATH ${FILE_PATH} DIRECTORY)
    list(APPEND ALL_INCLUDE_DIRS ${DIR_PATH})
endforeach()
list(REMOVE_DUPLICATES ALL_INCLUDE_DIRS)

target_include_directories(OmniBoxClient PRIVATE 
    "src"
    ${ALL_INCLUDE_DIRS}
)
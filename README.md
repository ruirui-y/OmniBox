# OmniBox

**OmniBox** 是一个由 **C++ Qt 客户端** 与 **自研微服务后端** 组成的跨平台设备互联与文件管理工具。客户端负责交互呈现，服务端通过自研 RPC 框架解耦为多个独立服务，实现设备发现、用户认证、文件元数据管理与高速传输。

## ✨ 核心功能

- **设备自动发现** —— 基于 UDP 广播，零配置感知局域网内其他 OmniBox 节点
- **用户认证体系** —— 账户登录、会话保持与心跳检测
- **文件元数据管理** —— 远程目录浏览、检索与结构展示
- **可靠文件传输** —— 支持文件上传 / 下载及传输进度反馈
- **Web 管理面板** —— 轻量原生 HTML/CSS/JS 界面，用于服务状态监控与基础管理
- **自研 RPC 通信** —— 全自研 TCP/HTTP 双通道异步 RPC 框架，无任何第三方网络库依赖

## 系统架构

OmniBox 采用 **Client / Microservices** 架构，客户端与服务端完全分离，通过 Protocol Buffers 序列化消息，自研 RPC 框架承载业务调用。

text

```
┌─────────────────────────────────────────────────────┐
│                  OmniBox Client (Qt6)                │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │ 登录界面  │ │ 设备列表  │ │ 文件视图  │ │ 传输管理│ │
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘ └────┬───┘ │
│        │              │              │             │    │
│  ┌─────┴──────────────┴──────────────┴─────────────┴──┐ │
│  │         ControHub（客户端核心调度与状态管理）        │ │
│  └───────────────────────┬───────────────────────────┘ │
│  ┌───────────────────────┴───────────────────────────┐ │
│  │      Qt UDP / TCP 网络层 (TCPMgr, UdpManager)     │ │
│  └───────────────────────┬───────────────────────────┘ │
└──────────────────────────┼────────────────────────────┘
                           │ Protobuf over TCP/HTTP
┌──────────────────────────┼────────────────────────────┐
│                  OmniBox Server (C++ Microservices)   │
│  ┌──────────┐ ┌──────────┐ ┌───────────┐ ┌──────────┐│
│  │gateway   │ │  login   │ │   meta    │ │ transfer ││
│  │ server   │ │  server  │ │   server  │ │  server  ││
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘ └─────┬────┘│
│        │              │              │              │    │
│  ┌─────┴──────────────┴──────────────┴──────────────┴──┐ │
│  │              自研 RPC 框架 (rpc_core)               │ │
│  │     TCP/HTTP 双通道 · 异步事件驱动 · 连接池管理      │ │
│  └───────────────────────┬────────────────────────────┘ │
│  ┌───────────────────────┴────────────────────────────┐ │
│  │              MySQL (用户 / 元数据持久化)             │ │
│  └────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```



## 服务详解

| 服务                | 职责                                            | 关键源码                                         |
| :------------------ | :---------------------------------------------- | :----------------------------------------------- |
| **gateway_server**  | 统一 HTTP/TCP 入口，流量路由与转发              | `GatewayHttpServer.cpp`, `GatewayTcpServer.cpp`  |
| **login_server**    | 用户登录认证、会话管理与心跳保活                | `MyLoginService.cpp`, `login_server.cpp`         |
| **meta_server**     | 文件元数据管理、目录浏览与检索                  | `MetaServiceImpl.cpp`, `meta_server.cpp`         |
| **transfer_server** | 文件上传/下载的实际数据传输                     | `TransferServiceImpl.cpp`, `transfer_server.cpp` |
| **rpc_core**        | 自研 RPC 框架，提供异步调用、连接池、序列化支持 | `RPCServer.h`, `MyChannel.cpp`                   |

所有服务均通过自研 RPC 框架进行内部通信，无任何第三方 RPC 库依赖。

## 技术栈

| 层级               | 技术                                                         | 说明                                                         |
| :----------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| **语言**           | C++ (85.6%), JavaScript (6.4%), C (3.4%), CSS (1.4%), HTML (0.8%) | 客户端与服务端均以现代 C++ 为主                              |
| **客户端 UI**      | Qt 6                                                         | 自绘组件库、QSS 样式系统、`.ui` 布局文件                     |
| **序列化**         | Protocol Buffers                                             | 定义所有通信协议，`.proto` 编译生成 C++ 代码                 |
| **RPC 框架**       | **自研 rpc_core**                                            | **完全自研**，无第三方依赖；支持 TCP/HTTP 双通道、异步事件驱动、连接池管理 |
| **数据库**         | MySQL                                                        | 持久化用户信息、文件元数据等                                 |
| **构建系统**       | CMake                                                        | 客户端与服务端均使用 CMake + CMakePresets 管理               |
| **网络层(客户端)** | Qt UDP / TCP Socket                                          | 封装 `TCPMgr`、`UdpManager`，处理设备发现与业务长连接        |
| **样式**           | Qt StyleSheet (QSS)                                          | 集中管理客户端视觉风格                                       |
| **Web 管理界面**   | 原生 HTML / CSS / JavaScript                                 | 位于 `www/` 目录，用于基础服务管理                           |

> ⚠️ 特别注意：RPC 框架 **完全未使用** libevent、gRPC 或任何其他外部网络库，全部基于标准 C++ 和操作系统 socket API 实现。

## 项目结构

text

```
OmniBox/
├── OmniBoxClient/               # Qt6 C++ 桌面客户端
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── StyleSheet/
│   │   └── stylesheet.qss       # 全局样式
│   ├── Images/                  # 图标、背景等资源
│   ├── ClientInstall/
│   │   └── Configs/
│   │       ├── config.json      # 运行配置
│   │       └── login.json       # 登录缓存
│   └── src/
│       ├── Global/              # 全局定义、工具类
│       ├── User/                # 用户状态管理
│       ├── TCP/                 # TCP 连接管理 (TCPMgr)
│       ├── UDP/                 # UDP 广播与设备发现 (UdpManager)
│       ├── Proto/               # Protobuf 协议定义
│       ├── Pb/                  # 由 .proto 生成的 C++ 代码
│       ├── ThreadPool/          # 通用线程池
│       ├── SELF_UI/             # 自绘 UI 组件
│       ├── ControHub/           # **客户端核心**：界面协调、设备列表管理、文件树状态等
│       ├── CustomUI/            # 自定义控件
│       ├── LoginWidget.cpp/h/ui # 登录窗口
│       └── ProtocolDef.h        # 协议常量
│
└── OmniBoxServer/               # C++ 微服务后端
    ├── CMakeLists.txt
    ├── proto/                   # 协议定义 (common, server_msg, internal_rpc)
    ├── pb/                      # 生成的 Protobuf 代码
    ├── rpc_core/                # **全自研 RPC 框架**
    │   ├── include/             # 头文件 (RPCServer, MyChannel, 连接池, 序列化辅助等)
    │   └── src/                 # 纯 C++ 实现
    ├── gateway_server/          # 网关服务
    ├── login_server/            # 登录服务
    ├── meta_server/             # 元数据服务
    ├── transfer_server/         # 文件传输服务
    └── www/                     # Web 管理面板 (HTML/CSS/JS)
```



## 快速开始

### 环境要求

- **操作系统**：Windows 10+, macOS 11+, Ubuntu 20.04+
- **编译器**：MSVC 2022 / GCC 11+ / Clang 14+
- **构建工具**：CMake ≥ 3.20
- **依赖库**：Qt 6.x, Protocol Buffers, MySQL Client
- **数据库**：MySQL 5.7+ (需预先创建数据库和用户)

### 构建步骤

bash

```
# 1. 克隆仓库
git clone https://github.com/ruirui-y/OmniBox.git
cd OmniBox

# 2. 构建服务端
cd OmniBoxServer
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 3. 构建客户端（需提前配置 Qt6 环境）
cd ../OmniBoxClient
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```



### 运行服务

1. 启动 MySQL 服务并导入相应的表结构（见 `OmniBoxServer/sql/` 目录下的初始化脚本）。
2. 启动各微服务（顺序不限）：

bash

```
./OmniBoxServer/build/gateway_server
./OmniBoxServer/build/login_server
./OmniBoxServer/build/meta_server
./OmniBoxServer/build/transfer_server
```



1. 启动客户端：

bash

```
./OmniBoxClient/build/OmniBoxClient
```



## 设计理念

OmniBox 严格遵循 **关注点分离** 原则，将“用户交互”与“业务服务”物理切分：

- **客户端**：纯 Qt 应用，专注于 **界面呈现** 与 **用户操作响应**。所有复杂逻辑（如文件索引、传输调度）均委托给后端服务，客户端仅通过简单的消息传递驱动界面状态。
- **服务端**：以 **微服务** 模式组织，每一个服务仅负责单一职责，并通过 **自研 RPC** 进行高效通信。服务之间无强依赖，可独立开发、测试与部署。
- **自研 RPC**：为了最大化控制通信开销与部署灵活性，我们没有引入任何第三方 RPC 库，而是从 socket 层开始构建了一套 **异步、双通道（TCP/HTTP）** 的轻量 RPC 框架，配合连接池与 Protobuf 序列化，专为 OmniBox 的场景深度定制。

这种架构使得客户端始终保持轻量化，服务端具备横向扩展能力，且核心通信组件的每一行代码都可掌控、可优化。

------

*OmniBox 是一个持续迭代的项目，未来将严格保持“客户端纯交互、服务端微服务化、通信自研可控”的设计主线，在设备互联与文件管理领域进行深度打磨。*
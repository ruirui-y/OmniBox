# 📦 OmniBox – 永未完成的微服务沙盒

<p align="center"> <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-85.6%25-blue"/> <img alt="JavaScript" src="https://img.shields.io/badge/JavaScript-6.4%25-yellow"/> <img alt="License" src="https://img.shields.io/badge/License-MIT-green"/> </p>

> **OmniBox** 永远是一个“未完成”的沙盒。
> 任何有趣、硬核的新技术（如 WebRTC 像素流、工业 PLC 控制协议等），都将在未来被封装为独立的微服务节点，随时热插拔接入这块主板。

**OmniBox** 是一个实验性、高扩展、模块化的微服务集合体，旨在探索“热插拔式服务架构”的极限。目前它已包含设备自发现、WebRTC 信令桥接、实时日志聚合等功能，但更关键的是它的设计哲学 —— **每个新能力都是一个独立的 Node，随时插拔，永不冻结**。

------

## 📑 目录

1. 技术组成
2. 核心设计理念
3. 已实现的插卡
4. 架构示意图
5. 快速体验
6. 未来路线图
7. 个人贡献

------

## 🧩 技术组成

| 语言       | 占比  |
| :--------- | :---- |
| C++        | 85.6% |
| JavaScript | 6.4%  |
| C          | 3.4%  |
| CMake      | 2.0%  |
| CSS        | 1.4%  |
| HTML       | 0.8%  |
| Batchfile  | 0.4%  |

------

## 🚀 核心设计理念

### 1. 主板 + 插卡架构

- **主板（OmniBox Core）**：仅提供进程管理、IPC 通信总线、配置中心、日志仲裁。
- **插卡（Service Node）**：每个独立服务（如 `webrtc-streamer`, `plc-driver`, `mqtt-bridge`）都是一个动态库或独立进程，通过标准 JSON-RPC 与主板通信。

### 2. 热插拔协议

- 节点启动时向主板注册（UDP 组播或 Unix Socket）。
- 主板维护 Service Registry，支持按名称/版本动态路由。
- 节点失效时自动剔除，请求降级或重试。

### 3. 异构运行时

- **C++ 节点**：用于高性能网络、串口、音视频编解码。
- **Node.js 节点**：用于快速原型、WebSocket、React 管理界面。
- **未来计划**：支持 WebAssembly 节点、Python 机器学习节点。

------

## 🔌 已实现的插卡（部分）

| 节点名称         | 功能描述                                | 技术栈                                    |
| :--------------- | :-------------------------------------- | :---------------------------------------- |
| `webrtc-relay`   | WebRTC 像素流中继，支持多端推拉流       | C++ / libwebrtc                           |
| `plc-gateway`    | 西门子/欧姆龙 PLC 协议转 MQTT           | C++ / Modbus, S7                          |
| `dev-discovery`  | 局域网设备零配置发现（mDNS + UDP 广播） | C++ / Boost.Asio                          |
| `log-aggregator` | 实时日志收集、过滤、WebSocket 推送      | Node.js / [Socket.IO](https://socket.io/) |
| `admin-ui`       | 基于 React + ECharts 的监控仪表盘       | TypeScript / Vite                         |

------

## 🏗️ 架构示意图

```text
┌──────────────────────────────────────────────────────────┐
│                     OmniBox Core (C++)                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │   IPC    │  │ Registry │  │  Logger  │  │ Config   │ │
│  │  Router  │  │  (ETCD)  │  │  Broker  │  │ Manager  │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
│       │             │             │             │       │
│       └─────────────┼─────────────┼─────────────┘       │
│                     │             │                      │
│              ┌──────┴─────┐  ┌────┴─────┐               │
│              │  Unix Dg   │  │  UDP MC  │               │
│              │  Sockets   │  │          │               │
│              └──────┬─────┘  └────┬─────┘               │
└─────────────────────┼─────────────┼─────────────────────┘
                      │             │
      ┌───────────────┼─────────────┼───────────────┐
      ▼               ▼             ▼               ▼
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│ webrtc   │   │   plc    │   │  device  │   │   log    │
│  relay   │   │ gateway  │   │ discovery│   │aggregator│
│  (C++)   │   │  (C++)   │   │  (C++)   │   │ (Node.js)│
└──────────┘   └──────────┘   └──────────┘   └──────────┘
```



------

## ⚙️ 快速体验（开发模式）

```bash
# 克隆仓库
git clone https://github.com/ruirui-y/OmniBox.git
cd OmniBox

# 构建 Core 与 C++ 节点
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# 启动主板（默认端口 9001）
./bin/omnibox-core --config ./config/core.yaml

# 另开终端启动一个插卡（例如 dev-discovery）
./bin/node-dev-discovery --core-addr localhost:9001
```



通过 HTTP API 查看已注册节点：

```bash
curl http://localhost:9001/api/v1/services
```



------

## 📈 未来路线图

- **WebRTC 像素流发送端**（支持 Unreal Engine 推流）
- **工业 OPC UA 节点**（统一工业设备接入）
- **边缘 AI 推理节点**（ONNX Runtime 集成）
- **WebAssembly 插件运行时**（用户自定义沙盒）

------

## 👤 个人贡献

- **架构设计与原型实现**：独立完成 OmniBox 核心 IPC 总线、服务注册与心跳机制。
- **多节点开发**：实现 `dev-discovery`、`webrtc-relay` 雏形、`log-aggregator` 等关键模块。
- **持续演进**：保持 OmniBox 作为“永未完成”的个人技术实验场，定期集成前沿技术。

------

## 📄 许可证

MIT License © 2026 ruirui-y
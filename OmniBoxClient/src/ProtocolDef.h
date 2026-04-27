#ifndef PROTOCOLDEF_H
#define PROTOCOLDEF_H

#include <QtGlobal>

// =========================================================================================
// 基础网络配置
// =========================================================================================
const quint16 UDP_SERVER_PORT = 9090;                                                   // 监听端口
const quint8  PROTOCOL_HEAD = 0xAE;                                                     // 协议固定头

// 设备类型 (0~中控, 1~坦克, 2~道具, 3~票机)
enum DeviceType {
    DevType_Center = 0,                                                                 // 中控主机
    DevType_Tank   = 1,                                                                 // 坦克从机
    DevType_Prop   = 2,                                                                 // 道具从机
    DevType_Ticket = 3                                                                  // 票机主控
};

// 协议核心命令码
enum ProtocolCmd {
    Cmd_Login        = 0x01,                                                            // 登录指令
    Cmd_SetInfo      = 0x02,                                                            // 设置设备信息
    Cmd_Heartbeat    = 0x03,                                                            // 心跳指令
    Cmd_StartGame    = 0x04,                                                            // 开始游戏
    Cmd_StopGame     = 0x05,                                                            // 停止游戏
    Cmd_TargetHit    = 0x06,                                                            // 标靶击中上报
    Cmd_PlaySound    = 0x07,                                                            // 音效播放
    Cmd_ParkQuery    = 0x08,                                                            // 泊车查询
    Cmd_Firmware     = 0x20                                                             // 固件升级
};

// =========================================================================================
// 统一的设备状态信息体 (用于内存级设备管理)
// =========================================================================================
struct DeviceInfo {
    quint8  uid;                                                                        // 设备唯一编号
    quint8  type;                                                                       // 设备类型
    QString uuid;                                                                       // 硬件 UUID
    QHostAddress ip;                                                                    // 设备当前网络 IP
    quint16 port;                                                                       // 设备当前网络端口
    qint64  lastActiveTime;                                                             // 最后活跃时间戳 (用于5秒离线检测)
    bool    isOnline;                                                                   // 当前在线状态
    
    // --- 坦克特有数据 ---
    quint8 tankHp = 100;                                                                // 坦克当前血量
    quint8 tankScore = 0;                                                               // 坦克当前分数
    quint8 tankTeam = 0;                                                                // 坦克所属队伍
    quint8 tankStatus = 0;                                                              // 坦克综合状态
    
    // --- 道具/票机特有数据 ---
    quint8 propStatus = 0;                                                              // 道具综合状态
    quint8 propLift = 0;                                                                // 升降状态 (0~降, 1~升)
};

Q_DECLARE_METATYPE(DeviceInfo)									                        // 声明为元类型
Q_DECLARE_METATYPE(QVector<DeviceInfo>)							                        // 如果信号/槽里用到了 QVector<DeviceInfo>
#endif // PROTOCOLDEF_H
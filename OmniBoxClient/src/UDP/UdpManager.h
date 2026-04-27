#ifndef UDPMANAGER_H
#define UDPMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QMap>
#include <QTimer>
#include "ProtocolDef.h"

class UdpManager : public QObject
{
    Q_OBJECT

public:
    explicit UdpManager(QObject* parent = nullptr);
    ~UdpManager();

public:
    void StartServer();                                                                 // 启动 UDP 监听服务

    // =====================================================================================
    // 对外暴露的控制接口 (UI层调用)
    // =====================================================================================
    void StartGame(quint8 uid, quint8 gameTime, quint8 hp, quint8 score, quint8 team);  // 下发开始游戏
    void StopGame(quint8 uid);                                                          // 下发停止游戏
    void PlaySound(quint8 uid, quint8 soundId);                                         // 触发音效播放
    void QueryParking();                                                                // 全局泊车查询

signals:
    // =====================================================================================
    // 向上层UI发送的事件信号
    // =====================================================================================
    void DeviceOnlineChanged(quint8 uid, quint8 type, bool isOnline);                   // 设备在线/离线状态改变
    void TargetHitEvent(quint8 tankUid, quint8 weaponType);                             // 标靶被击中事件
    void ParkingEvent(quint8 parkId, QByteArray rfid);                                  // 泊车状态改变事件

private slots:
    void onReadyRead();                                                                 // Socket 数据接收槽
    void onTimer1000ms();                                                               // 1秒精度的定时器 (处理心跳与离线)

private:
    // 核心协议封包与拆包引擎
    void ProcessDatagram(const QByteArray& data, const QHostAddress& addr, quint16 port);
    void SendPacket(quint8 type, quint8 uid, quint8 cmd, const QByteArray& params, const QHostAddress& addr, quint16 port);
    quint8 CalcChecksum(const QByteArray& data);                                        // 计算通信校验和

    // 具体业务命令处理
    void HandleLogin(quint8 type, quint8 uid, const QByteArray& params, const QHostAddress& addr, quint16 port);
    void HandleHeartbeatAck(quint8 uid, const QByteArray& params);                      // 处理从机心跳回应

    // 机制辅助函数
    quint8 AllocateTankUid();                                                           // 坦克动态分配UID策略

private:
    QUdpSocket* m_socket;                                                               // 核心通信套接字
    QTimer* m_timer;                                                                    // 轮询定时器
    int m_heartbeatTick = 0;                                                            // 心跳周期计数器

    QMap<quint8, DeviceInfo> m_devices;                                                 // 中控维护的所有设备映射表
};

#endif // UDPMANAGER_H
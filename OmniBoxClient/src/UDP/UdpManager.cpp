#include "UdpManager.h"
#include <QDateTime>
#include <QDebug>

UdpManager::UdpManager(QObject* parent) : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    m_timer = new QTimer(this);

    connect(m_socket, &QUdpSocket::readyRead, this, &UdpManager::onReadyRead);
    connect(m_timer, &QTimer::timeout, this, &UdpManager::onTimer1000ms);
}

UdpManager::~UdpManager()
{
}

void UdpManager::StartServer()
{
    if (m_socket->bind(QHostAddress::Any, UDP_SERVER_PORT)) {
        qDebug() << "UDP Server Started on port:" << UDP_SERVER_PORT;
        m_timer->start(1000);                                                           // 启动 1 秒滴答定时器
    }
    else {
        qDebug() << "UDP Server Bind Failed!";
    }
}

// =========================================================================================
// 1. 数据接收与解析层
// =========================================================================================
void UdpManager::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());

        QHostAddress senderIp;
        quint16 senderPort;

        m_socket->readDatagram(datagram.data(), datagram.size(), &senderIp, &senderPort);
        ProcessDatagram(datagram, senderIp, senderPort);
    }
}

void UdpManager::ProcessDatagram(const QByteArray& data, const QHostAddress& addr, quint16 port)
{
    int size = data.size();

    // 最小长度限制: 头(1) + 长度(2) + 类型(1) + UID(1) + 命令(1) + 校验(1) = 7字节
    if (size < 7 || (quint8)data[0] != PROTOCOL_HEAD) return;

    // 长度字段校验: 长度值 = 类型 + UID + 命令 + 参数 + 校验 的字节总数
    quint16 len = ((quint8)data[1] << 8) | (quint8)data[2];
    if (size != len + 3) return;                                                        // 封包总长 = 头(1) + 长度本身(2) + len

    // 校验和验证 (对包体内容进行累加)
    quint8 receivedSum = data[size - 1];
    quint8 calcSum = CalcChecksum(data.mid(3, len - 1));
    if (receivedSum != calcSum) {
        qDebug() << "Checksum Error!";
        return;
    }

    // 提取协议核心字段
    quint8 type = data[3];
    quint8 uid = data[4];
    quint8 cmd = data[5];
    QByteArray params = data.mid(6, len - 4);                                           // 提取纯参数区

    // 刷新设备活跃时间戳
    if (m_devices.contains(uid)) {
        m_devices[uid].lastActiveTime = QDateTime::currentMSecsSinceEpoch();
        if (!m_devices[uid].isOnline) {
            m_devices[uid].isOnline = true;
            emit DeviceOnlineChanged(uid, type, true);                                  // 触发重连上线信号
        }
    }

    // 命令业务路由
    switch (cmd) {
    case Cmd_Login:
        HandleLogin(type, uid, params, addr, port);
        break;
    case Cmd_Heartbeat:
        HandleHeartbeatAck(uid, params);
        break;
    case Cmd_TargetHit:
        if (params.size() >= 2) {
            emit TargetHitEvent(params[0], params[1]);                              // 抛出击中事件
        }
        break;
    case Cmd_ParkQuery:
        if (params.size() >= 2) {
            emit ParkingEvent(params[0], params.mid(1));                            // 抛出泊车事件
        }
        break;
    default:
        break;
    }
}

// =========================================================================================
// 2. 登录与动态 UID 分配机制
// =========================================================================================
void UdpManager::HandleLogin(quint8 type, quint8 uid, const QByteArray& params, const QHostAddress& addr, quint16 port)
{
    if (params.size() < 10) return;                                                     // UUID(4) + HW(3) + FW(3)
    QString uuidHex = params.left(4).toHex();

    quint8 assignedUid = uid;

    // 0xFF 表示新设备首次登录，需要由中控动态分配 UID
    if (uid == 0xFF) {
        if (type == DevType_Tank) {
            assignedUid = AllocateTankUid();                                            // 坦克动态分配 1~63
        }
        else {
            qDebug() << "Error: Prop/Ticket should have static UID.";
            return;
        }
    }

    // 在内存中注册或更新设备信息
    DeviceInfo info;
    info.uid = assignedUid;
    info.type = type;
    info.uuid = uuidHex;
    info.ip = addr;
    info.port = port;
    info.lastActiveTime = QDateTime::currentMSecsSinceEpoch();
    info.isOnline = true;

    m_devices[assignedUid] = info;
    emit DeviceOnlineChanged(assignedUid, type, true);                                  // 通知UI层设备上线

    // 向从机回应分配成功的 UID
    QByteArray respParams;
    respParams.append(assignedUid);
    SendPacket(DevType_Center, 0, Cmd_Login, respParams, addr, port);
}

quint8 UdpManager::AllocateTankUid()
{
    for (quint8 i = 1; i <= 63; ++i) {                                                  // 坦克编号池 1~63
        if (!m_devices.contains(i)) return i;                                           // 找到空闲坑位即返回
    }
    return 0;                                                                           // 编号池已满
}

// =========================================================================================
// 3. 心跳下发与死亡检测 (1Hz滴答)
// =========================================================================================
void UdpManager::onTimer1000ms()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_heartbeatTick++;

    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        DeviceInfo& dev = it.value();

        // 超过 5 秒未收到数据，下达离线判决
        if (now - dev.lastActiveTime > 5000) {
            if (dev.isOnline) {
                dev.isOnline = false;
                emit DeviceOnlineChanged(dev.uid, dev.type, false);                     // 通知UI层设备掉线
            }
            continue;                                                                   // 离线设备停止下发心跳
        }

        // 满 2 秒下发一次周期心跳
        if (m_heartbeatTick % 2 == 0) {
            QByteArray hbParams;

            // 针对不同硬件类型组装差异化心跳参数
            if (dev.type == DevType_Tank) {
                hbParams.append(dev.tankStatus);
                hbParams.append((char)0x00);                                            // 开关状态占位
                hbParams.append(dev.tankTeam);
                hbParams.append(dev.tankHp);
                hbParams.append(dev.tankScore);
            }
            else if (dev.type == DevType_Prop) {
                hbParams.append(dev.propStatus);
                hbParams.append((char)0x00);                                            // 开关状态占位
                hbParams.append(dev.propLift);
            }
            else if (dev.type == DevType_Ticket) {
                hbParams.append((char)0x00);                                            // 状态占位
                hbParams.append((char)0x00);                                            // 开关状态占位
                hbParams.append(20, (char)0x00);                                        // 10台票机的累计数 (填充20字节)
            }

            SendPacket(DevType_Center, dev.uid, Cmd_Heartbeat, hbParams, dev.ip, dev.port);
        }
    }
}

void UdpManager::HandleHeartbeatAck(quint8 uid, const QByteArray& params)
{
    // 处理从机回传的心跳参数，例如更新电量、时间等
    // if (m_devices[uid].type == DevType_Tank && params.size() >= 3) { ... }
}

// =========================================================================================
// 4. API 接口与底层组包发送引擎
// =========================================================================================
void UdpManager::StartGame(quint8 uid, quint8 gameTime, quint8 hp, quint8 score, quint8 team)
{
    if (!m_devices.contains(uid)) return;

    QByteArray params;
    params.append(gameTime).append(hp).append(score).append(team);
    SendPacket(DevType_Center, uid, Cmd_StartGame, params, m_devices[uid].ip, m_devices[uid].port);
}

void UdpManager::StopGame(quint8 uid)
{
    if (!m_devices.contains(uid)) return;

    QByteArray params;
    params.append((char)0x00);                                                          // 停止指令参数为0
    SendPacket(DevType_Center, uid, Cmd_StopGame, params, m_devices[uid].ip, m_devices[uid].port);
}

void UdpManager::PlaySound(quint8 uid, quint8 soundId)
{
    if (!m_devices.contains(uid)) return;

    QByteArray params;
    params.append(soundId);
    SendPacket(DevType_Center, uid, Cmd_PlaySound, params, m_devices[uid].ip, m_devices[uid].port);
}

void UdpManager::QueryParking()
{
    // 全局泊车位查询逻辑
}

// 核心组包与网络发送
void UdpManager::SendPacket(quint8 type, quint8 uid, quint8 cmd, const QByteArray& params, const QHostAddress& addr, quint16 port)
{
    QByteArray packet;
    packet.append(PROTOCOL_HEAD);                                                       // 塞入协议头 0xAE

    // 计算包体长度: 类型(1) + UID(1) + 命令(1) + 参数(N) + 校验(1)
    quint16 len = 1 + 1 + 1 + params.size() + 1;
    packet.append((len >> 8) & 0xFF);                                                   // 写入长度高位
    packet.append(len & 0xFF);                                                          // 写入长度低位

    // 组装校验数据区
    QByteArray checkData;
    checkData.append(type).append(uid).append(cmd).append(params);

    packet.append(checkData);
    packet.append(CalcChecksum(checkData));                                             // 追加校验和计算结果

    m_socket->writeDatagram(packet, addr, port);                                        // 将完整数据包打入网络
}

quint8 UdpManager::CalcChecksum(const QByteArray& data)
{
    quint8 sum = 0;
    for (char c : data) {
        sum += (quint8)c;                                                               // 字节级累加
    }
    return sum;
}
#include "TCPMgr.h"
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "Macro.h"
#include "Global.h"

TCPMgr::TCPMgr(QObject* parent) : QObject(parent)
{
    m_Host = "127.0.0.1";
    m_Port = 5486;

    InitTcpSocket();
    InitHearbeatTimer();
    InitHandlers();                                                                     // 注册路由表
    qDebug() << "Tcp Thread id = " << QThread::currentThread()->objectName();
}

TCPMgr::~TCPMgr()
{
    m_bIsInitiateDisCon = true;
    if (m_TcpSocket) {
        m_TcpSocket->abort();
        m_TcpSocket->deleteLater();
    }
    if (m_hearbeatTimer) {
        m_hearbeatTimer->stop();
        m_hearbeatTimer->deleteLater();
    }
}

// =========================================================================================
// 1. 初始化层
// =========================================================================================
void TCPMgr::InitTcpSocket()
{
    m_TcpSocket = new QTcpSocket(this);

    // 连接成功
    connect(m_TcpSocket, &QTcpSocket::connected, this, [this]() {
        qDebug() << u8"[TCPMgr] 连接服务器成功:" << m_Host << m_Port;
        m_buffer.clear();                                                               // 清空历史缓存，防止脏数据
        emit SigConnectSuccess(true);
        });

    // 连接断开
    connect(m_TcpSocket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << u8"[TCPMgr] 连接已断开";
        StopHeartBeat();
        emit SigConnectClose();

        // 如果不是主动断开，可以考虑在这里做断线重连逻辑
        if (!m_bIsInitiateDisCon) {
            // QTimer::singleShot(3000, this, &TCPMgr::SlotTcpConnect);
        }
        });

    // 核心：数据到达
    connect(m_TcpSocket, &QTcpSocket::readyRead, this, &TCPMgr::onReadyRead);

    // 错误处理
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_TcpSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError err) {
        qDebug() << "[TCPMgr] Socket Error:" << err << m_TcpSocket->errorString();
        });
#else
    connect(m_TcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [this](QAbstractSocket::SocketError err) {
        qDebug() << "[TCPMgr] Socket Error:" << err << m_TcpSocket->errorString();
        });
#endif
}

void TCPMgr::InitHearbeatTimer()
{
    m_hearbeatTimer = new QTimer(this);
    connect(m_hearbeatTimer, &QTimer::timeout, this, &TCPMgr::onHeartbeatTick);
}

// =========================================================================================
// 2. 路由注册层 (业务分发中心)
// =========================================================================================
void TCPMgr::InitHandlers()
{
    // ------------------------------------------------------------------
    // 注册 [登录响应] 的处理逻辑
    // ------------------------------------------------------------------
    m_router[ServerApi::ID_LOGIN_RSP] = [this](const ServerApi::PacketHeader& header, const QByteArray& bodyData) {
        // 1. 检查 Header 里的全局错误码
        if (header.error_code() != ServerApi::ErrorCode::ERR_SUCCESS) 
        {
            qDebug() << u8"[TCPMgr] 登录失败:" << header.error_msg().c_str();
            emit SigLoginFailed(header.error_code(), QString::fromStdString(header.error_msg()));
            return;
        }

        // 2. 错误码为0，解析 Body 拿业务数据
        ServerApi::LoginRsp rsp;
        if (rsp.ParseFromArray(bodyData.data(), bodyData.size()))
        {
            qDebug() << u8"[TCPMgr] 登录成功! 服务器时间:" << rsp.server_time();

            StartHeartBeat();                                                           // 登录成功后启动心跳保活
            emit SigLoginSuccess();
        }
        };

    // ------------------------------------------------------------------
    // 注册 [心跳响应] 的处理逻辑
    // ------------------------------------------------------------------
    m_router[ServerApi::ID_HEARTBEAT] = [this](const ServerApi::PacketHeader& header, const QByteArray& bodyData) {
        // 一般心跳不需要复杂处理，只需知道服务器活着即可
        // qDebug() << "[TCPMgr] 收到服务器心跳回应";
        };
}

// =========================================================================================
// 3. 核心：拆包与分发引擎 (彻底解决粘包/半包)
// =========================================================================================
void TCPMgr::onReadyRead()
{
    // 将底层缓冲区的所有数据追加到我们的缓存中
    m_buffer.append(m_TcpSocket->readAll());

    // 只要缓存区大于等于 4 字节，说明至少包含了一个 TotalLen 的头
    while (m_buffer.size() >= 4) {
        QDataStream stream(&m_buffer, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::BigEndian);                                    // 统一大端网络字节序

        quint32 totalLen;
        stream >> totalLen;                                                             // 读出此包的总长度

        // 如果缓冲区的数据不够一个完整的包，说明是"半包"，跳出循环等待后续数据到达
        if (m_buffer.size() < totalLen) {
            break;
        }

        // --- 此时我们已经拥有了一个完美、完整的物理包！ ---

        quint16 headerLen;
        stream >> headerLen;                                                            // 读出 Header 的长度 (2字节)

        // 截取 RpcHeader 的字节流，并反序列化
        QByteArray headerData = m_buffer.mid(6, headerLen);
        ServerApi::PacketHeader header;

        if (header.ParseFromArray(headerData.data(), headerData.size())) {
            // 根据公式算出 Body 的长度，并截取 Body
            int bodyLen = totalLen - 4 - 2 - headerLen;
            QByteArray bodyData = m_buffer.mid(6 + headerLen, bodyLen);

            // O(1) 复杂度的极速路由分发
            ServerApi::MsgId msgId = header.msg_id();
            if (m_router.contains(msgId)) {
                m_router[msgId](header, bodyData);                                      // 触发你写的 Lambda 回调
            }
            else {
                qDebug() << u8"[TCPMgr] 未知的 MsgId:" << msgId << u8"丢弃该数据包";
            }
        }
        else {
            qDebug() << u8"[TCPMgr] 包头 Protobuf 解析失败，数据可能被篡改！";
        }

        // 从缓存中剔除已经处理完的这个包，进入下一个循环处理“粘包”
        m_buffer.remove(0, totalLen);
    }
}

// =========================================================================================
// 4. 核心：多线程安全的封包发送引擎
// =========================================================================================
void TCPMgr::SendProtoMsg(ServerApi::MsgId msgId, const google::protobuf::Message& protoMsg, uint64_t seqId)
{
    // 1. 将业务 Message 序列化为 Body 字节流
    QByteArray bodyData;
    bodyData.resize(protoMsg.ByteSizeLong());
    protoMsg.SerializeToArray(bodyData.data(), bodyData.size());

    // 2. 组装并序列化 PacketHeader
    ServerApi::PacketHeader header;
    header.set_msg_id(msgId);
    header.set_seq_id(seqId);
    // error_code 客户端发给服务端一般默认0，由服务端填错误码

    QByteArray headerData;
    headerData.resize(header.ByteSizeLong());
    header.SerializeToArray(headerData.data(), headerData.size());

    // 3. 计算总长度：TotalLen(4) + HeaderLen(2) + HeaderSize + BodySize
    quint32 totalLen = 4 + 2 + headerData.size() + bodyData.size();

    // 4. 拼装最终的二进制 TCP 数据流
    QByteArray finalPacket;
    QDataStream stream(&finalPacket, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << totalLen;                                                                 // 写入 4 字节总长
    stream << (quint16)headerData.size();                                               // 写入 2 字节头长
    finalPacket.append(headerData);                                                     // 拼接 Header 数据
    finalPacket.append(bodyData);                                                       // 拼接 Body 数据

    // 5. 跨线程安全的发送机制
    // 不管你在哪个子线程调用 TCPMgr::Instance()->SendProtoMsg，它都会安全地切回主线程去调用 socket 写入
    QMetaObject::invokeMethod(this, [this, finalPacket, msgId]() {
        if (m_TcpSocket && m_TcpSocket->state() == QAbstractSocket::ConnectedState)
        {
            m_TcpSocket->write(finalPacket);
            m_TcpSocket->flush();
        }
        else 
        {
            qDebug() << u8"[TCPMgr] 发送失败，TCP 未连接！MsgId:" << finalPacket.mid(6, 2).toHex();
        }
        }, Qt::QueuedConnection);
}

// =========================================================================================
// 5. 业务操作控制层
// =========================================================================================
void TCPMgr::SlotTcpConnect()
{
    if (m_TcpSocket->state() != QAbstractSocket::ConnectedState) {
        m_bIsInitiateDisCon = false;
        qDebug() << u8"[TCPMgr] 正在连接服务器...";
        m_TcpSocket->connectToHost(m_Host, m_Port);
    }
}

void TCPMgr::Login(QString username, QString password)
{
    ServerApi::LoginReq login_req;
    login_req.set_username(username.toStdString());
    login_req.set_password(password.toStdString());

    SendProtoMsg(ServerApi::MsgId::ID_LOGIN_REQ, login_req);
}

void TCPMgr::AccountLoginOut()
{
    m_bIsInitiateDisCon = true;
    StopHeartBeat();
    if (m_TcpSocket) {
        m_TcpSocket->disconnectFromHost();
    }
}

void TCPMgr::StartHeartBeat()
{
    if (m_hearbeatTimer && !m_hearbeatTimer->isActive()) {
        m_hearbeatTimer->start(5000);                                                   // 每 5 秒发一次心跳
    }
}

void TCPMgr::StopHeartBeat()
{
    if (m_hearbeatTimer && m_hearbeatTimer->isActive()) {
        m_hearbeatTimer->stop();
    }
}

void TCPMgr::onHeartbeatTick()
{
    // 发送心跳包
    ServerApi::Heartbeat hb;
    hb.set_timestamp(QDateTime::currentMSecsSinceEpoch());

    SendProtoMsg(ServerApi::ID_HEARTBEAT, hb);
}
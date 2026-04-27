#ifndef STRUCT_H
#define STRUCT_H

#include <QPixmap>
#include <QString>
#include <QHostAddress>
#include <QByteArray>

/* 服务器信息 */
struct ServerInfo {
	QString Host;
	QString Port;
};

/* 聊天信息 */
struct MsgInfo {
	QString MsgType;					// 文件，图片，文本
	QString MsgContent;					// 文本内容
	QPixmap MsgPicture;					// 图片内容
	QString Msg_Image_Data;				// 图片二进制数据
};

/* UDP信息 */
struct UDPMessage
{
public:
	uint16_t ID;						// 消息ID
	uint16_t MsgLen;
	QString Msg;						// 发送时使用
	QByteArray Data;					// 数据
	QHostAddress Addr;
	int Port;
};

/* 发送消息 */
struct UDPSendMessage : public UDPMessage
{

};

/* 接收消息 */
struct UDPRecvMessage : public UDPMessage
{

};

/* 游戏信息 */
struct GameInfo
{
	QString id;
	QString title;
	QString coverPath;
	int minPlayers = 1;
	int maxPlayers = 4;
	bool installed = true;
	bool comingSoon = false;
};

/* 游戏数据 */
struct GameData
{
	QString GameName;											// 游戏名字
	int MinPlayers;												// 最少玩家人数
	int MaxPlayers;												// 支持最大玩家人数
};

/* 用户信息 */
struct UserInfo
{
	QString UserName;											// 用户名
	QString Password;											// 密码

public:
	void ResetData()
	{
		UserName.clear();
		Password.clear();
	}
};


/* 游戏海报 */
struct GamePoster
{
	QString cover;												// 大图
	QString thumb;												// 缩略图
};

#endif // STRUCT_H

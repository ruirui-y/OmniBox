#ifndef MACRO_H
#define MACRO_H

#define MSG_TYPE_TEXT									"text"
#define MSG_TYPE_IMAGE									"image"
#define MSG_TYPE_FILE									"file"

#define CHAT_COUNT_PER_PAGE								13
#define CONTACT_COUNT_PER_PAGE							13

#define MAX_TEXT_LEN									1024

// 线程数量
#define MAX_THREAD_COUNT								5

// 心跳间隔
#define HEART_BEAT_INTERVAL								5000

// 超时检测时间
#define HEARTBEAT_TIMEOUT								10000

// IP地址
#define IP_ADDRESS										"175.178.36.122"

// TCP端口
#define TCP_SERVER_PORT									5482

// UDP端口
#define UDP_BROADCASTER_PORT							8889

// 返回结果
#define RETURN_SUCCESS									"SUCCESS"
#define RETURN_FAILURE									"FAILURE"

// JSON的键
#define MAP_SIZE										"MapSize"
#define ADDRESS											"Address"
#define NICK											"Nick"
#define TEAM											"Team"

// 是和否
#define YES												"YES"
#define NO												"NO"

// Windows设备ID
#define WINDOWS_DEVICE_ID								"WINDOWS"

// 侧边栏的宽度
#define HZ_LIST_WIDTH                                   180

// 安装游戏阻塞时间以及执行普通的ADB查询阻塞时间
#define ADB_BLOCK_TIME_INSTALL_APK						60000
#define ADB_BLOCK_TIME_QUERY							8000

#endif // MACRO_H
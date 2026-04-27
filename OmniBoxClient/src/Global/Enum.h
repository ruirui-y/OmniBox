#ifndef ENUM_H
#define ENUM_H

enum ReqID_TCP
{
	ID_GET_VARIFY_CODE                      = 1001,			    // 获取验证码
	ID_REG_USER		                        = 1002,			    // 注册用户
    ID_RESET_PWD                            = 1003,             // 重置密码
    ID_LOGIN_USER                           = 1004,             // 登录用户
    ID_LOGIN_REQUEST                        = 1005,             // 登录请求
    ID_LOGIN_RESPONSE                       = 1006,             // 登录响应
    ID_NOTIFY_OFF_LINE_REQUEST              = 1007,				// 通知用户下线请求
    ID_NOTIFY_OFF_LINE_RESPONSE             = 1008,				// 通知用户下线响应
    ID_ADD_USER_VR_DEVICE_REQUEST           = 1009,				// 添加用户VR设备请求
    ID_ADD_USER_VR_DEVICE_RESPONSE          = 1010,				// 添加用户VR设备响应
    ID_DEL_USER_VR_DEVICE_REQUEST           = 1011,				// 删除用户VR设备请求
    ID_DEL_USER_VR_DEVICE_RESPONSE          = 1012,				// 删除用户VR设备响应
    ID_CHECK_VR_DEVICE_REQUEST              = 1013,				// 检查VR设备请求
    ID_CHECK_VR_DEVICE_RESPONSE             = 1014,				// 检查VR设备响应
    ID_UPDATE_DEVICE_NICK_REQUEST           = 1015,				// 更新VR设备昵称请求
    ID_UPDATE_DEVICE_NICK_RESPONSE          = 1016,				// 更新VR设备昵称响应
    ID_USER_OFF_LINE_REQUEST                = 1017,				// 用户下线请求
    ID_USER_OFF_LINE_RESPONSE               = 1018,				// 用户下线响应
    ID_USER_HEART_BEAT_REQUEST              = 1019,				// 用户心跳请求
    ID_USER_HEART_BEAT_RESPONSE             = 1020,				// 用户心跳响应
};

enum ReqID_VR
{
    ID_VR_DEVICE_ID_REQUEST                 = 600,              // VR设备ID请求
    ID_VR_DEVICE_ID_RESPONSE                = 601,              // VR设备ID响应
    ID_VR_START_GAME_REQUEST                = 602,              // VR游戏启动请求
    ID_VR_START_GAME_RESPONSE               = 603,              // VR游戏启动响应
    ID_VR_STOP_GAME_REQUEST                 = 604,              // VR游戏停止请求
    ID_VR_STOP_GAME_RESPONSE                = 605,              // VR游戏停止响应
    ID_HEART_BEAT_REQUEST                   = 606,			    // 心跳请求
    ID_HEART_BEAT_RESPONSE                  = 607,			    // 心跳回复
    ID_VR_BROCAST_MSG_REQUEST               = 608,			    // VR广播消息请求
    ID_VR_BROCAST_MSG_RESPONSE              = 609,			    // VR广播消息回复
    ID_VR_MODIFY_USER_NICK_REQUEST          = 610,              // VR修改昵称请求
    ID_VR_MODIFY_USER_NICK_RESPONSE         = 611,              // VR修改昵称响应
    ID_VR_MODIFY_USER_TEAM_REQUEST          = 612,              // VR修改队伍请求
    ID_VR_MODIFY_USER_TEAM_RESPONSE         = 613,              // VR修改队伍响应
};

enum Modules
{
	REGISTERMOD		                        = 0,				// 注册模块
	LOGINMOD		                        = 1,				// 登录模块
};

enum ErrorCodes
{
    SUCCESS                                 = 0,
    LOGIN_USER_EXIT_ERR                     = 1,                // 登录失败
    LOGIN_PWD_ERR                           = 2,                // 密码错误
    LOGIN_USER_NOT_EXIST_ERR                = 3,                // 用户不存在
    JSON_ERR                                = 4,                // JSON解析失败
    REMOVE_DEVICE_ERR                       = 5,                // 删除设备失败
    ADD_DEVICE_ERR                          = 6,                // 添加设备失败
};

/* 标签错误类型 */
enum TipErr {
    TIP_SUCCESS                             = 0,
    TIP_EMAIL_ERR                           = 1,
    TIP_PWD_ERR                             = 2,
    TIP_CONFIRM_ERR                         = 3,
    TIP_PWD_CONFIRM                         = 4,
    TIP_VERIFY_ERR                          = 5,
    TIP_USER_ERR                            = 6
};

enum ClickLabelState
{
    Normal = 0,                                                 // 普通状态
    Selected = 1,                                               // 选中状态
};


// 自定义QListWidgetItem的几种类型
enum ListItemType {
    CHAT_USER_ITEM,                                             // 聊天用户
    CONTACT_USER_ITEM,                                          // 联系人用户
    SEARCH_USER_ITEM,                                           // 搜索到的用户
    ADD_USER_TIP_ITEM,                                          // 提示添加用户
    INVALID_ITEM,                                               // 不可点击条目
    GROUP_TIP_ITEM,                                             // 分组提示条目
    LINE_ITEM,                                                  // 分割线
    APPLY_FRIEND_ITEM,                                          // 好友申请
};

// 聊天界面模式
enum DemandUIMode {
    SearchMode,                                                 // 搜索模式
    InstallMode,                                                // 安装模式
    MainMode,                                                   // 联系模式
    SettingsMode,                                               // 设置模式
};

// 角色类型
enum ChatRole
{
    Self,                                                       // 自己
    Other,                                                      // 其他人
};

// 界面状态
enum UIStatus
{
    LOGIN_STATUS,                                               // 登录状态
    REGISTER_STATUS,                                            // 注册状态
    RESET_STATUS,                                               // 重置密码状态
    CHAT_STATUS,                                                // 聊天状态
};

/* 游戏卡片信息 */
enum GameCardInfo
{
    IdRole = Qt::UserRole + 1,                                  // 游戏唯一ID（QString）。双击/启动等逻辑用它定位条目
    TitleRole,                                                  // 标题（QString），卡片下方大写文字
    CoverRole,                                                  // 封面（通常 QString：资源/文件路径；也可用 QPixmap，但务必统一）
    MinPlayersRole,                                             // 最小玩家数（int）
    MaxPlayersRole,                                             // 最大玩家数（int）
    InstalledRole,                                              // 是否已安装（bool）
    ComingSoonRole                                              // 是否即将上线（bool）
};

#endif // ENUM_H
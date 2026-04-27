#ifndef MSG_ID_H
#define MSG_ID_H

#include <cstdint>

namespace game 
{
namespace net 
{

enum MsgID : uint32_t 
{
    // 登录模块 (1000起步)
    MSG_LOGIN_REQ = 1001,
    MSG_LOGIN_RESP = 1002,

    // 匹配模块 (2000起步)
    MSG_JOIN_MATCH_REQ = 2001,
    MSG_JOIN_MATCH_RESP = 2002,
    MSG_MATCH_SUCCESS_PUSH = 2003,

    // 聊天模块 (3000起步)
    MSG_CHAT_REQ = 3001,
    MSG_CHAT_RESP = 3002,
    MSG_CHAT_PUSH = 3003,  // 这个对应 ClientChatPush
};

} // namespace net
} // namespace game

#endif
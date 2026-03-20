#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class ConnectionPool 
{
public:
    static ConnectionPool& GetInstance() 
    {
        static ConnectionPool pool;
        return pool;
    }

    // 核心方法：获取一个可用的连接
    int GetConnection(std::string ip, uint16_t port) 
    {
        std::string key = ip + ":" + std::to_string(port);
        std::lock_guard<std::mutex> lock(m_mutex);

        // 1. 查找是否已有连接
        auto it = m_clientMap.find(key);
        if (it != m_clientMap.end()) 
        {
            int fd = it->second;
            // 2. 存活校验：发一个空包探测连接是否还在
            if (send(fd, NULL, 0, MSG_NOSIGNAL) >= 0) 
            {
                return fd;
            }
            else 
            {
                // 连接已断开，清理掉并重新创建
                close(fd);
                m_clientMap.erase(it);
            }
        }

        // 3. 创建新连接 (池子里没有或旧的坏了)
        int clientfd = socket(AF_INET, SOCK_STREAM, 0);
        if (clientfd == -1) return -1;

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
        {
            close(clientfd);
            return -1;
        }

        // 4. 存入 Map 并返回
        m_clientMap[key] = clientfd;
        return clientfd;
    }

    // 归还连接：在 KeepAlive 模式下保持在 Map 里不关
    void ReleaseConnection(std::string ip, uint16_t port, int fd)
    {
        // 目前长连接逻辑下，直接留着即可
    }

private:
    ConnectionPool() {}
    ~ConnectionPool() {
        for (auto& pair : m_clientMap) {
            if (pair.second != -1) {
                close(pair.second);
            }
        }
    }
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    std::unordered_map<std::string, int> m_clientMap;
    std::mutex m_mutex;
};

#endif
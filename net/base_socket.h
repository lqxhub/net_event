#pragma once

#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <string>
#include <functional>
#include <sys/socket.h>

#include "net_event.h"
#include "base_event.h"

static constexpr int SOCKET_WIN_SIZE = 128 * 1024;

struct SocketAddr {
    SocketAddr() { Clear(); }

    SocketAddr(const SocketAddr &other) { memcpy(&addr_, &other.addr_, sizeof addr_); }

    SocketAddr &operator=(const SocketAddr &other) {
        if (this != &other) {
            memcpy(&addr_, &other.addr_, sizeof addr_);
        }
        return *this;
    }

    SocketAddr(const sockaddr_in &addr) { Init(addr); }

    SocketAddr(uint32_t netip, uint16_t netport) { Init(netip, netport); }

    SocketAddr(const std::string &ip, uint16_t hostport) { Init(ip, hostport); }

    void Init(const sockaddr_in &addr) { memcpy(&addr_, &addr, sizeof(addr)); }

    void Init(uint32_t netIp, uint16_t netPort) {
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = netIp;
        addr_.sin_port = netPort;
    }

    void Init(const std::string &ip, uint16_t hostPort) {
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = ::inet_addr(ip.data());
        addr_.sin_port = htons(hostPort);
    }

    const sockaddr_in &GetAddr() const { return addr_; }

    inline std::string GetIP() const { return ::inet_ntoa(addr_.sin_addr); }

    inline std::string GetIP(char *buf, socklen_t size) const {
        return ::inet_ntop(AF_INET, reinterpret_cast<const char *>(&addr_.sin_addr), buf, size);
    }

    inline uint16_t GetPort() const { return ntohs(addr_.sin_port); }

    inline bool Empty() const { return 0 == addr_.sin_family; }

    void Clear() { memset(&addr_, 0, sizeof addr_); }

    inline friend bool operator==(const SocketAddr &a, const SocketAddr &b) {
        return a.addr_.sin_family == b.addr_.sin_family && a.addr_.sin_addr.s_addr == b.addr_.sin_addr.s_addr &&
               a.addr_.sin_port == b.addr_.sin_port;
    }

    inline friend bool operator!=(const SocketAddr &a, const SocketAddr &b) { return !(a == b); }

    sockaddr_in addr_{};
};


class BaseSocket : public NetEvent {

public:
    enum {
        SOCKET_NONE = 0,//error socket
        SOCKET_TCP,
        SOCKET_UDP,
        SOCKET_LISTEN_TCP,
        SOCKET_LISTEN_UDP,
    };

    BaseSocket(int fd) : NetEvent(fd) {}

    ~BaseSocket() override = default;

    void OnError() override {};

    void Close() override;

    static int CreateTCPSocket();

    static int CreateUDPSocket();


    // Called when the socket is created
    void OnCreate();

    void SetNonBlock(bool noBlock);

    void SetNodelay();

    void SetSndBuf(socklen_t size = SOCKET_WIN_SIZE);

    void SetRcvBuf(socklen_t size = SOCKET_WIN_SIZE);

    void SetReuseAddr();

    bool SetReusePort();

    bool GetLocalAddr(SocketAddr &);

    bool GetPeerAddr(SocketAddr &);


    inline int SocketType() { return type_; }

    inline void SetSocketType(int type) { type_ = type; }

protected:
    inline bool NoBlock() const {
        return noBlock_;
    }

private:
    int type_ = 0;//socket type (TCP/UDP)
    bool noBlock_ = true;
};

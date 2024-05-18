
#include <netinet/tcp.h>

#include "config.h"
#include "listen_socket.h"
#include "stream_socket.h"

const int ListenSocket::LISTENQ = 1024;

bool ListenSocket::REUSE_PORT = true;

int ListenSocket::OnReadable(const std::shared_ptr<Connection> &conn, std::string *readBuff) {
    auto newConnFd = Accept();
    if (newConnFd == 0) {
        return NE_ERROR;
    }

    auto newConn = std::make_unique<StreamSocket>(newConnFd, SocketType());

    newConn->OnCreate();
    conn->netEvent_ = std::move(newConn);
    conn->fd_ = newConnFd;

    return newConnFd;
}

int ListenSocket::OnWritable() {
    return 1;
}

bool ListenSocket::SendPacket(std::string &&msg) {
    return false;
}

int ListenSocket::Init() {
    if (!Open()) {
        return static_cast<int>(NetListen::OPEN_ERROR);
    }
    if (!Bind()) {
        return static_cast<int>(NetListen::BIND_ERROR);
    }

    if (!Listen()) {
        return static_cast<int>(NetListen::LISTEN_ERROR);
    }
    return static_cast<int>(NetListen::OK);
}

bool ListenSocket::Open() {
    if (Fd() != 0) {
        return false;
    }

    if (addr_.Empty()) {
        return false;
    }

    if (SocketType() == SOCKET_LISTEN_TCP) {
        fd_ = CreateTCPSocket();
    } else if (SocketType() == SOCKET_LISTEN_UDP) {
        fd_ = CreateUDPSocket();
    } else {
        return false;
    }
    return true;
}

bool ListenSocket::Bind() {
    if (Fd() == 0) {
        return false;
    }

    fd_ = CreateTCPSocket();
    SetNonBlock(true);
    SetNodelay();
    SetReuseAddr();
    if (!SetReusePort()) {
        REUSE_PORT = true;
    }

    struct sockaddr_in serv = addr_.GetAddr();

    int ret = ::bind(Fd(), reinterpret_cast<struct sockaddr *>(&serv), sizeof serv);
    if (0 != ret) {
        Close();
        return false;
    }
    return true;
}

bool ListenSocket::Listen() {
    int ret = ::listen(Fd(), ListenSocket::LISTENQ);
    if (0 != ret) {
        Close();
        return false;
    }
    return true;
}

int ListenSocket::Accept() {
    socklen_t addrLength = sizeof(addr_);
#ifdef HAVE_ACCEPT4
    return ::accept4(Fd(), reinterpret_cast<struct sockaddr *>(&addr_), &addrLength, SOCK_NONBLOCK);
#else
    return ::accept(Fd(), reinterpret_cast<struct sockaddr *>(&addr_), &addrLength);
#endif
}

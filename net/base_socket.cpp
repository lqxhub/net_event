
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "base_socket.h"

int BaseSocket::CreateTCPSocket() {
    return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int BaseSocket::CreateUDPSocket() {
    return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

void BaseSocket::Close() {
    auto fd = Fd();
    if (fd_.compare_exchange_strong(fd, 0)) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }
}

void BaseSocket::OnCreate() {
#ifndef HAVE_ACCEPT4
    SetNonBlock(true);
#endif
    SetNodelay();
    SetSndBuf();
    SetRcvBuf();
}

void BaseSocket::SetNonBlock(bool noBlock) {
    int flag = ::fcntl(Fd(), F_GETFL, 0);
    if (-1 == flag) {
        return;
    }

    if (noBlock) {
        if (::fcntl(Fd(), F_SETFL, flag | O_NONBLOCK) != -1) {
            noBlock_ = true;
        }
    } else {
        if (::fcntl(Fd(), F_SETFL, flag & ~O_NONBLOCK) != -1) {
            noBlock_ = false;
        }
    }
}

void BaseSocket::SetNodelay() {
    int nodelay = 1;
    ::setsockopt(Fd(), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&nodelay), sizeof(int));
}

void BaseSocket::SetSndBuf(socklen_t winsize) {
    ::setsockopt(Fd(), SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char *>(&winsize), sizeof(winsize));
}

void BaseSocket::SetRcvBuf(socklen_t winsize) {
    ::setsockopt(Fd(), SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char *>(&winsize), sizeof(winsize));
}

void BaseSocket::SetReuseAddr() {
    int reuse = 1;
    ::setsockopt(Fd(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));
}

bool BaseSocket::SetReusePort() {
    int reuse = 1;
    return ::setsockopt(Fd(), SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&reuse), sizeof(reuse)) != -1;
}

bool BaseSocket::GetLocalAddr(SocketAddr &addr) {
    sockaddr_in localAddr{};
    socklen_t len = sizeof(localAddr);

    if (0 == ::getsockname(Fd(), reinterpret_cast<struct sockaddr *>(&localAddr), &len)) {
        addr.Init(localAddr);
    } else {
        return false;
    }

    return true;
}

bool BaseSocket::GetPeerAddr(SocketAddr &addr) {
    sockaddr_in remoteAddr{};
    socklen_t len = sizeof(remoteAddr);
    if (0 == ::getpeername(Fd(), reinterpret_cast<struct sockaddr *>(&remoteAddr), &len)) {
        addr.Init(remoteAddr);
    } else {
        return false;
    }

    return true;
}

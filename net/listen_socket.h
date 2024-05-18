#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <atomic>
#include <memory>

#include "base_socket.h"

class ListenSocket : public BaseSocket {

public:
    static ListenSocket *CreateTCPListen() {
        return new ListenSocket(SOCKET_LISTEN_TCP);
    }

    static ListenSocket *CreateUDPListen() {
        return new ListenSocket(SOCKET_LISTEN_UDP);
    }

    static const int LISTENQ;
    static bool REUSE_PORT;// Determine whether REUSE_PORT can be used

    inline void SetListenAddr(const SocketAddr &addr) {
        addr_ = addr;
    }

    // Accept new connection and create new connection object
    // when the connection is established, the OnCreate function is called
    int OnReadable(const std::shared_ptr<Connection> &conn, std::string *readBuff) override;

    // The function is cant be used
    int OnWritable() override;

    // The function is cant be used
    bool SendPacket(std::string &&msg) override;

    // Initialize the socket and bind the address
    int Init() override;

private:
    ListenSocket(int type) : BaseSocket(0) {
        SetSocketType(type);
    }

    // Open the socket
    bool Open();

    // Bind the address
    bool Bind();

    // Start listening
    bool Listen();

private:
    // Accept new connection
    int Accept();

    SocketAddr addr_; // Listen address
};

#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <atomic>
#include <memory>
#include <mutex>

#include "base_socket.h"

class StreamSocket : public BaseSocket {

public:
    StreamSocket(int fd, int type) : BaseSocket(fd) {
        SetSocketType(type);
    }

    int Init() override { return 1; };

    int OnReadable(const std::shared_ptr<Connection> &conn, std::string *readBuff) override;

    int OnWritable() override;

    bool SendPacket(std::string &&msg) override;

    int Read(std::string *readBuff);

private:
    const int readBuffSize_ = 4 * 1024;//read from socket buff size 4K

    std::mutex sendMutex_;//send data buff mutex

    std::string sendData_;//send data buff
    size_t sendPos_ = 0;//send data buff pos
};

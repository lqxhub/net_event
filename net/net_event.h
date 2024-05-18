#pragma once

#include <atomic>

#include "callback_function.h"

// For human readability
enum {
    NE_ERROR = -1,
    NE_OK = 1,
};


enum class NetListen {
    OK = 0,
    OPEN_ERROR,
    BIND_ERROR,
    LISTEN_ERROR,
};

//对所有网络的抽象
class NetEvent {

public:
    NetEvent(int fd) : fd_(fd) {}

    virtual ~NetEvent() = default;

    // Initialize the event
    virtual int Init() = 0;

    // Handle read event when the connection is readable and the data can be read
    virtual int OnReadable(const std::shared_ptr<Connection> &conn, std::string *readBuff) = 0;

    // Handle write event when the connection is writable and the data can be sent
    virtual int OnWritable() = 0;

    virtual void OnError() = 0;

    // Send data
    virtual bool SendPacket(std::string &&msg) = 0;

    virtual void Close() = 0;

    inline int Fd() const {
        return fd_.load();
    }

protected:
    std::atomic<int> fd_ = 0;
};


#pragma once

#include <atomic>
#include <thread>

#include "base_event.h"

class IOThread {
public:
    IOThread(const std::shared_ptr<BaseEvent> &event) : baseEvent_(event) {};

    ~IOThread() = default;

    // Initialize the event and run the event loop
    bool Run();

    inline void CloseConnection(int fd) { baseEvent_->DelEvent(fd); }

    // Stop the event loop and wait for the thread to exit
    void Stop();

    // Wait for the thread to exit
    void Wait();

    // Add read event to epoll when send message to client
    inline void SetWriteEvent(int fd) {
        baseEvent_->AddWriteEvent(fd);
    }

    // Add new event to epoll when new connection
    inline void AddNewEvent(int fd, int mask) {
        baseEvent_->AddEvent(fd, mask);
    }

protected:
    std::atomic<bool> running_ = true;

    std::thread thread_;

    std::shared_ptr<BaseEvent> baseEvent_;// Event object
};


//For now, there is no need to distinguish between read and write threads


//class IOReadThread : public IOThread {
//public:
//    IOReadThread(const std::shared_ptr<BaseEvent> &event) : IOThread(event) {};
//
//    ~IOReadThread() override = default;
//
//    bool Run() override;
//};
//
//class IOWriteThread : public IOThread {
//public:
//    IOWriteThread(const std::shared_ptr<BaseEvent> &event) : IOThread(event) {};
//
//    ~IOWriteThread() override = default;
//
//    bool Run() override;
//};

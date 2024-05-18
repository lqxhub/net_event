#pragma once

#include "config.h"

#ifdef HAVE_EPOLL

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <utility>

#include "base_event.h"


class EpollEvent : public BaseEvent {

public:
    explicit EpollEvent(const std::shared_ptr<NetEvent> &listen, int8_t mode) : BaseEvent(listen, mode,
                                                                                          BaseEvent::EVENT_TYPE_EPOLL) {
    };

    ~EpollEvent() override {
        Close();
    }

    // Initialize the epoll event
    bool Init() override;

    // Add event to epoll, mask is the event type
    void AddEvent(int fd, int mask) override;

    // Delete event from epoll
    void DelEvent(int fd) override;

    // Poll event
    void EventPoll() override;

    // Add write event to epoll
    void AddWriteEvent(int fd) override;

    // Delete write event from epoll
    void DelWriteEvent(int fd) override;

    // Handle read event
    void EventRead();

    // Handle write event
    void EventWrite();

    // Do read event
    void DoRead(const epoll_event &event, const std::shared_ptr<Connection> &conn);

    // Do write event
    void DoWrite(const epoll_event &event, const std::shared_ptr<Connection> &conn);

    // Handle error event
    void DoError(const epoll_event &event, std::string &&err);

private:
    const int eventsSize = 1024;
};

#endif

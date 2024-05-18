#pragma once

#include "config.h"

#ifdef HAVE_KQUEUE

#include <unistd.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

#include "base_event.h"

class KqueueEvent : public BaseEvent {
public:
    explicit KqueueEvent(std::shared_ptr<NetEvent> listen, int8_t mode) : BaseEvent(std::move(listen), mode,
                                                                                    BaseEvent::EVENT_TYPE_KQUEUE) {
    };

    ~KqueueEvent() override {
        Close();
    }

    bool Init() override;

    void AddEvent(int fd, int mask) override;

    void DelEvent(int fd) override;

    void AddWriteEvent(int fd) override;

    void DelWriteEvent(int fd) override;

    void EventPoll() override;

    void EventRead();

    void EventWrite();

    void DoRead(const struct kevent &event, const std::shared_ptr<Connection> &conn);

    void DoWrite(const struct kevent &event, const std::shared_ptr<Connection> &conn);

    void DoError(const struct kevent &event, std::string &&err);

private:
    const int eventsSize = 1020;
};

#endif
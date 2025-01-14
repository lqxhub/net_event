
#include "kqueue_event.h"

#ifdef HAVE_KQUEUE

const int BaseEvent::EVENT_READ = EVFILT_READ;
const int BaseEvent::EVENT_WRITE = EVFILT_WRITE;
const int BaseEvent::EVENT_ERROR = EV_ERROR;
const int BaseEvent::EVENT_HUB = EV_EOF;

bool KqueueEvent::Init() {
    fd_ = kqueue();
    if (fd_ == -1) {
        return false;
    }
    if (mode_ & EVENT_MODE_READ) {
        AddEvent(listen_->Fd(), EVENT_READ);
    }
    pipe(pipeFd);
    AddEvent(pipeFd[0], EVENT_READ | EVENT_ERROR | EVENT_HUB);
    return true;
}

void KqueueEvent::AddEvent(int fd, int mask) {
    struct kevent change;
    EV_SET(&change, fd, mask, EV_ADD, 0, 0, nullptr);
    kevent(Fd(), &change, 1, nullptr, 0, nullptr);
}

void KqueueEvent::DelEvent(int fd) {
    struct kevent change;
    EV_SET(&change, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    kevent(Fd(), &change, 1, nullptr, 0, nullptr);
}

void KqueueEvent::AddWriteEvent(int fd) {
    AddEvent(fd,EVENT_WRITE);
}

void KqueueEvent::DelWriteEvent(int fd) {
    struct kevent change;
    EV_SET(&change, fd, EVENT_WRITE, EV_DELETE, 0, 0, nullptr);
    kevent(Fd(), &change, 1, nullptr, 0, nullptr);
}

void KqueueEvent::EventPoll() {
    if (mode_ & EVENT_MODE_READ) {
        EventRead();
    } else {
        EventWrite();
    }
}

void KqueueEvent::EventRead() {
    struct kevent events[eventsSize];
    while (running_) {
        int nev = kevent(Fd(), nullptr, 0, events, eventsSize, nullptr);
        for (int i = 0; i < nev; ++i) {
            if ((events[i].flags & EVENT_HUB) || (events[i].flags & EVENT_ERROR)) {
                DoError(events[i], "");
                continue;
            }
            std::shared_ptr<Connection> conn;
            if (events[i].filter == EVENT_READ) {
                if (events[i].ident != listen_->Fd()) {
                    conn = getConn_(events[i].ident);
                }
                DoRead(events[i], conn);
            } else if ((mode_ & EVENT_MODE_WRITE) && events[i].filter == EVENT_WRITE) {
                conn = getConn_(events[i].ident);
                if (!conn) {
                    DoError(events[i], "write conn is null");
                    continue;
                }
                DoWrite(events[i], conn);
            }
        }
    }
}

void KqueueEvent::EventWrite() {
    struct kevent events[eventsSize];
    while (running_) {
        int nev = kevent(Fd(), nullptr, 0, events, eventsSize, nullptr);
        for (int i = 0; i < nev; ++i) {
            if ((events[i].flags & EVENT_HUB) || (events[i].flags & EVENT_ERROR)) {
                DoError(events[i], "EventWrite error");
                continue;
            }
            auto conn = getConn_(events[i].ident);
            if (!conn) {
                DoError(events[i], "write conn is null");
                continue;
            }
            if (events[i].filter == EVENT_WRITE) {
                DoWrite(events[i], conn);
            }
        }
    }
}

void KqueueEvent::DoRead(const struct kevent &event, const std::shared_ptr<Connection> &conn) {
    if (event.ident == listen_->Fd()) {
        auto newConn = std::make_shared<Connection>(shared_from_this(), nullptr);
        auto connFd = listen_->OnReadable(newConn, nullptr);
        onCreate_(connFd, newConn);
    } else if (conn) {
        std::string readBuff;
        int ret = conn->netEvent_->OnReadable(conn, &readBuff);

        onMessage_(event.ident, std::move(readBuff));
    } else {
        DoError(event, "DoRead error");
    }
}

void KqueueEvent::DoWrite(const struct kevent &event, const std::shared_ptr<Connection> &conn) {
    auto ret = conn->netEvent_->OnWritable();
    if (ret == NE_ERROR) {
        DoError(event, "DoWrite error");
        return;
    }
    if (ret == 0) {
        DelWriteEvent(event.ident);
    }
}

void KqueueEvent::DoError(const struct kevent &event, std::string &&err) {
    DelEvent(event.ident);
    onClose_(event.ident, std::move(err));
}

#endif
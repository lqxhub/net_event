
#include "epoll_event.h"

#ifdef HAVE_EPOLL

#include "callback_function.h"

const int BaseEvent::EVENT_READ = EPOLLIN;
const int BaseEvent::EVENT_WRITE = EPOLLOUT;
const int BaseEvent::EVENT_ERROR = EPOLLERR;
const int BaseEvent::EVENT_HUB = EPOLLHUP;

bool EpollEvent::Init() {
    fd_ = epoll_create1(0);
    if (fd_ == -1) {// If the epoll creation fails, return false
        return false;
    }
    if (mode_ & EVENT_MODE_READ) {// Add the listen socket to epoll for read
        AddEvent(listen_->Fd(), EVENT_READ | EVENT_ERROR | EVENT_HUB);
    }
    pipe(pipeFd);
    AddEvent(pipeFd[0], EVENT_READ | EVENT_ERROR | EVENT_HUB);

    return true;
}

void EpollEvent::AddEvent(int fd, int mask) {
    struct epoll_event ev{};
    ev.events = mask;
    ev.data.fd = fd;
    epoll_ctl(Fd(), EPOLL_CTL_ADD, fd, &ev);
}

void EpollEvent::DelEvent(int fd) {
    epoll_ctl(Fd(), EPOLL_CTL_DEL, fd, nullptr);
}

void EpollEvent::EventPoll() {
    if (mode_ & EVENT_MODE_READ) {// If it is a read multiplex, call EventRead
        EventRead();
    } else {// If it is a write multiplex, call EventWrite
        EventWrite();
    }
}

void EpollEvent::AddWriteEvent(int fd) {
    if (mode_ & EVENT_MODE_READ) {// If it is a read multiplex, modify the event
        struct epoll_event ev{};
        ev.events = EVENT_WRITE;
        ev.data.fd = fd;
        epoll_ctl(Fd(), EPOLL_CTL_MOD, fd, &ev);
    } else {// If it is a write multiplex, add the event
        struct epoll_event ev{};
        ev.events = EVENT_WRITE;
        ev.data.fd = fd;
        epoll_ctl(Fd(), EPOLL_CTL_ADD, fd, &ev);
    }
}

void EpollEvent::DelWriteEvent(int fd) {
    if (mode_ & EVENT_MODE_READ) {// If it is a read multiplex, modify the event to read
        struct epoll_event ev{};
        ev.events = EVENT_READ;
        ev.data.fd = fd;
        epoll_ctl(Fd(), EPOLL_CTL_MOD, fd, &ev);
    } else {
        struct epoll_event ev{};
        ev.data.fd = fd;
        epoll_ctl(Fd(), EPOLL_CTL_DEL, fd, &ev);
    }
}

void EpollEvent::EventRead() {
    struct epoll_event events[eventsSize];
    while (running_) {
        int nfds = epoll_wait(Fd(), events, eventsSize, -1);
        for (int i = 0; i < nfds; ++i) {
            if ((events[i].events & EVENT_HUB) || (events[i].events & EVENT_ERROR)) {
                // If the event is an error event, call DoError
                DoError(events[i], "");
                continue;
            }
            std::shared_ptr<Connection> conn;
            if (events[i].events & EVENT_READ) {
                // If the event is less than the listen socket, it is a new connection
                if (events[i].data.fd != listen_->Fd()) {
                    conn = getConn_(events[i].data.fd);
                }
                DoRead(events[i], conn);
            }

            if ((mode_ & EVENT_MODE_WRITE) && events[i].events & EVENT_WRITE) {
                conn = getConn_(events[i].data.fd);
                if (!conn) {// If the connection is empty, call DoError
                    DoError(events[i], "connection is null");
                    continue;
                }
                // If the event is a write event, call DoWrite
                DoWrite(events[i], conn);
            }
        }
    }
}

void EpollEvent::EventWrite() {
    struct epoll_event events[eventsSize];
    while (running_) {
        int nfds = epoll_wait(Fd(), events, eventsSize, -1);
        for (int i = 0; i < nfds; ++i) {
            if ((events[i].events & EVENT_HUB) || (events[i].events & EVENT_ERROR)) {
                DoError(events[i], "");
            }
            auto conn = getConn_(events[i].data.fd);
            if (!conn) {
                DoError(events[i], "connection is null");
                continue;
            }
            if (events[i].events & EVENT_WRITE) {
                DoWrite(events[i], conn);
            }
        }
    }
}

void EpollEvent::DoRead(const epoll_event &event, const std::shared_ptr<Connection> &conn) {
    if (event.data.fd == listen_->Fd()) {
        auto newConn = std::make_shared<Connection>(shared_from_this(), nullptr);
        auto connFd = listen_->OnReadable(newConn, nullptr);
        if (connFd < 0) {
            DoError(event, "accept error");
            return;
        }
        onCreate_(connFd, newConn);
    } else if (conn) {
        std::string readBuff;
        int ret = conn->netEvent_->OnReadable(conn, &readBuff);
        if (ret == NE_ERROR) {
            DoError(event, "read error");
            return;
        }
        onMessage_(event.data.fd, std::move(readBuff));
    } else {
        DoError(event, "connection is null");
    }
}

void EpollEvent::DoWrite(const epoll_event &event, const std::shared_ptr<Connection> &conn) {
    auto ret = conn->netEvent_->OnWritable();
    if (ret == NE_ERROR) {
        DoError(event, "write error");
        return;
    }
    if (ret == 0) {
        DelWriteEvent(event.data.fd);
    }
}

void EpollEvent::DoError(const epoll_event &event, std::string &&err) {
    DelEvent(event.data.fd);
    onClose_(event.data.fd, std::move(err));
}

#endif
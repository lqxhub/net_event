
#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <condition_variable>

#include "base_socket.h"
#include "io_thread.h"
#include "callback_function.h"
#include "listen_socket.h"
#include "thread_manager.h"

template<typename T> requires HasSetFdFunction<T>
class EventServer final {
public:
    explicit EventServer(int8_t threadNum) : threadNum_(threadNum) {
        threadsManager_.reserve(threadNum_);
    }

    ~EventServer() = default;

    inline void SetOnCreate(OnCreate<T> &&func) {
        OnCreate_ = std::move(func);
    }

    inline void SetOnMessage(OnMessage<T> &&func) {
        OnMessage_ = std::move(func);
    }

    inline void SetOnClose(OnClose<T> &&func) {
        OnClose_ = std::move(func);
    }

    inline void AddListenAddr(const SocketAddr &addr) {
        listenAddrs_ = addr;
    }

    inline void SetRwSeparation(bool separation = true) {
        rwSeparation_ = separation;
    }

    std::pair<bool, std::string> StartServer();

    // Stop the server
    void StopServer();

    // Send message to the client
    void SendPacket(const T &conn, std::string &&msg);

    // Server Active close the connection
    void CloseConnection(const T &conn);

    // When the service is started, the main thread is blocked,
    // and when all the subthreads are finished, the function unblocks and returns
    void Wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !running_.load(); });
    }

private:
    int Main();

private:
    OnCreate<T> OnCreate_;// The callback function when the connection is created

    OnMessage<T> OnMessage_; // The callback function when the message is received

    OnClose<T> OnClose_; // The callback function when the connection is closed

    SocketAddr listenAddrs_; // The address to listen on

    std::atomic<bool> running_ = true; // Whether the server is running

    bool rwSeparation_ = true;// Whether to separate read and write

    int8_t threadNum_ = 1;// The number of threads

    std::vector<std::unique_ptr<ThreadManager<T>>> threadsManager_;

    std::mutex mtx_;
    std::condition_variable cv_;
};

template<typename T>
requires HasSetFdFunction<T>
std::pair<bool, std::string> EventServer<T>::StartServer() {
    if (threadNum_ <= 0) {
        return std::pair(false, "thread num must be greater than 0");
    }

    if (!OnCreate_) {
        return std::pair(false, "OnCreate_ must be set");
    }

    if (!OnMessage_) {
        return std::pair(false, "OnMessage_ must be set");
    }

    if (!OnClose_) {
        return std::pair(false, "OnClose_ must be set");
    }

    for (int8_t i = 0; i < threadNum_; ++i) {
        auto tm = std::make_unique<ThreadManager<T>>(i, rwSeparation_);
        tm->SetOnCreate(OnCreate_);
        tm->SetOnMessage(OnMessage_);
        tm->SetOnClose(OnClose_);
        threadsManager_.emplace_back(std::move(tm));
    }

    if (Main() != static_cast<int>(NetListen::OK)) {
        return std::pair(false, "Main function error");
    }

    return std::pair(true, "");
}


template<typename T>
requires HasSetFdFunction<T>
void EventServer<T>::StopServer() {
    bool expected = true;
    if (running_.compare_exchange_strong(expected, false)) {
        for (const auto &thread: threadsManager_) {
            thread->Stop();
        }
    }
    cv_.notify_one();
}

template<typename T>
requires HasSetFdFunction<T>
void EventServer<T>::SendPacket(const T &conn, std::string &&msg) {
    int thIndex = -1;
    if constexpr (IsPointer_v<T>) {
        thIndex = conn->GetThreadIndex();
    } else {
        thIndex = conn.GetThreadIndex();
    }
    threadsManager_[thIndex]->SendPacket(conn, std::move(msg));
}

template<typename T>
requires HasSetFdFunction<T>
void EventServer<T>::CloseConnection(const T &conn) {
    int thIndex = -1;
    if constexpr (IsPointer_v<T>) {
        thIndex = conn->GetThreadIndex();
    } else {
        thIndex = conn.GetThreadIndex();
    }
    threadsManager_[thIndex]->CloseConnection(conn.GetFd());
}

template<typename T>
requires HasSetFdFunction<T>int EventServer<T>::Main() {
    std::shared_ptr<ListenSocket> listen(ListenSocket::CreateTCPListen());
    listen->SetListenAddr(listenAddrs_);

    if (auto ret = listen->Init() != static_cast<int>(NetListen::OK)) {
        return ret;
    }

    int i = 0;
    for (const auto &thread: threadsManager_) {
        if (i > 0 && ListenSocket::REUSE_PORT) {
            listen.reset(ListenSocket::CreateTCPListen());
            listen->SetListenAddr(listenAddrs_);
            if (auto ret = listen->Init() != static_cast<int>(NetListen::OK)) {
                return ret;
            }
        }
        if (!thread->Start(listen)) {
            return -1;
        }
        ++i;
    }

    return static_cast<int>(NetListen::OK);
}

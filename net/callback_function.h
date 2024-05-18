#pragma once

#include <functional>
#include <memory>

template<typename T>
struct IsPointer : std::false_type {
};

template<typename T>
struct IsPointer<T *> : std::true_type {
};

template<typename T>
struct IsPointer<std::shared_ptr<T>> : std::true_type {
};

template<typename T>
struct IsPointer<std::unique_ptr<T>> : std::true_type {
};

template<typename T>
constexpr bool IsPointer_v = IsPointer<T>::value;

template<typename T>
concept HasSetFdFunction = requires(T t, int id, int8_t index) {
    // If T is of pointer type, then dereference and call the member function
    { (*t).SetFd(id) } -> std::same_as<void>; // SetFd return type is void
    { (*t).GetFd() } -> std::same_as<int>;    // GetFd return type is int
    { (*t).SetThreadIndex(index) } -> std::same_as<void>; // SetThreadIndex return type is void
    { (*t).GetThreadIndex() } -> std::same_as<int8_t>;     // GetThreadIndex return type is int8_t
} || std::is_class_v<T>; // If T is an ordinary class, the member function is called directly

template<typename T> requires HasSetFdFunction<T>
using OnCreate = std::function<void(int fd, T *t)>;

template<typename T> requires HasSetFdFunction<T>
using OnMessage = std::function<void(std::string &&msg, T &t)>;

template<typename T> requires HasSetFdFunction<T>
using OnClose = std::function<void(T & t, std::string && err)>;

class BaseEvent;

class NetEvent;

// Auxiliary structure
struct Connection {
    explicit Connection(const std::shared_ptr<BaseEvent> &poll, std::unique_ptr<NetEvent> netEvent)
            : poll_(poll), netEvent_(std::move(netEvent)) {}

    ~Connection() = default;

    std::shared_ptr<BaseEvent> poll_;
    std::unique_ptr<NetEvent> netEvent_;

    int fd_ = 0;
};
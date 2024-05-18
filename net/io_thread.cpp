
#include "io_thread.h"

void IOThread::Stop() {
    if (!running_.load()) {
        return;
    }

    running_ = false;
    baseEvent_->Close();
    Wait();
}

void IOThread::Wait() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool IOThread::Run() {
    if (!baseEvent_->Init()){
        return false;
    }

    thread_ = std::thread([this] {
        baseEvent_->EventPoll();
    });
    return true;
}

//bool IOReadThread::Run() {
//    if (!IOThread::Run()) {
//        return false;
//    }
//    thread_ = std::thread([this] {
//        baseEvent_->EventPoll();
//    });
//
//    return true;
//}
//
//bool IOWriteThread::Run() {
//    if (!IOThread::Run()) {
//        return false;
//    }
//    thread_ = std::thread([this] {
//        baseEvent_->EventPoll();
//    });
//    return true;
//}

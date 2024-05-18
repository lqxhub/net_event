
#include "stream_socket.h"

int StreamSocket::OnReadable(const std::shared_ptr<Connection> &conn, std::string *readBuff) {
    return Read(readBuff);
}

//return bytes that have not yet been sent
int StreamSocket::OnWritable() {
    std::lock_guard<std::mutex> lock(sendMutex_);
    size_t ret = ::write(Fd(), sendData_.c_str() + sendPos_, sendData_.size() - sendPos_);
    if (ret == -1) {
        return NE_ERROR;
    }
    sendPos_ += ret;
    if (sendPos_ == sendData_.size()) {
        sendPos_ = 0;
        sendData_.clear();
        return 0;
    }
    return static_cast<int>(sendData_.size() - sendPos_);
}

bool StreamSocket::SendPacket(std::string &&msg) {
    std::lock_guard<std::mutex> lock(sendMutex_);
    sendData_.append(msg);
    return true;
}

// Read data from the socket
int StreamSocket::Read(std::string *readBuff) {
    char readBuffer[readBuffSize_];
    while (true) {
        int ret = static_cast<int>(::read(Fd(), readBuffer, readBuffSize_));
        if (ret == -1) {
            if (EAGAIN == errno || EWOULDBLOCK == errno || ECONNRESET == errno) {
                return NE_OK;
            } else {
                return NE_ERROR;
            }
        }
        if (!NoBlock() || ret == 0) {
            break;
        }
        if (ret > 0) {
            readBuff->append(readBuffer, ret);
        }
    }

    return NE_OK;
}

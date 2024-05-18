#include <iostream>
#include <execinfo.h>
#include <cxxabi.h>
#include <csignal>
#include <memory>

#include "event_server.h"

class Client {
public:

    inline void SetFd(int fd) {
        fd_ = fd;
    }

    inline void SetThreadIndex(int8_t index) {
        threadIndex_ = index;
    }

    inline int GetFd() const {
        return fd_;
    }

    inline int8_t GetThreadIndex() const {
        return threadIndex_;
    }

private:
    int fd_;
    int8_t threadIndex_;
};

std::unique_ptr<EventServer<std::shared_ptr<Client>>> g_server;

void signalHandler(int signum) {
    std::cout << "Received signal " << signum << ". Exiting...\n";
    // stop the network server
    g_server->StopServer();
}

int main() {
    g_server = std::make_unique<EventServer<std::shared_ptr<Client>>>(2);

    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    SocketAddr addr("0.0.0.0", 8088);
    g_server->AddListenAddr(addr);

    g_server->SetOnCreate([](int fd, std::shared_ptr<Client> *client) {
        *client = std::make_shared<Client>();
        std::cout << "id: " << fd << " Create connection" << std::endl;
    });
    g_server->SetOnMessage([](std::string &&msg, std::shared_ptr<Client> &client) {
        std::cout << "id:" << client->GetFd() << " Receive message: " << msg << std::endl;
        std::string str("to client\n");
        for (int i = 0; i < 10000; ++i) {
            str += "to client ";
        }
        str += " end \n";
        g_server->SendPacket(client, std::move(str));
    });
    g_server->SetOnClose([](std::shared_ptr<Client> &client, std::string &&err) {
        std::cout << "id: " << client->GetFd() << " Close connection" << " err:" << err << std::endl;
    });

    g_server->SetRwSeparation(true);

    auto ret = g_server->StartServer();
    if (!ret.first) {
        std::cerr << ret.second << std::endl;
        return -1;
    } else {
        std::cout << "Server start success" << std::endl;
    }

    g_server->Wait();

    return 0;
}

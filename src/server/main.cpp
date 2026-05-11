#include "cluster_state.hpp"
#include "mi_config.hpp"
#include "worker/WorkerManager.hpp"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

static int open_server_socket() {
    auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        throw std::runtime_error("could not initialize socket");
    }

    // SO_REUSEADDR нужен, чтобы когда я постоянно киляю сервер или падает на сегфолте
    // я мог быстро запустить сервер без лизинга ядром.
    const int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        throw std::runtime_error("could not setsockopt(SO_REUSEADDR)");
    }

    addrinfo hints = {
        // AI_PASSIVE - для bind, AI_NUMERICSERV - не разрешать имена
        .ai_flags = AI_PASSIVE | AI_NUMERICSERV,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,

        // Занулены
        .ai_addrlen = 0,
        .ai_addr = 0,
        .ai_canonname = 0,
        .ai_next = nullptr,
    };

    addrinfo *s_i = nullptr;
    if (int ret = getaddrinfo(nullptr, std::to_string(mi::Config::Port).c_str(), &hints, &s_i);
        ret != 0) {
        throw std::logic_error(gai_strerror(ret));
    }

    if (bind(sock, s_i->ai_addr, s_i->ai_addrlen) < 0) {
        throw std::logic_error("could not bind address");
    }

    freeaddrinfo(s_i);

    if (listen(sock, 128) < 0) {
        throw std::logic_error("could not listen");
    }

    return sock;
}

static volatile bool stop_requested = false;

static void sigint_handler(int) {
    stop_requested = true;
    kill(getpid(), SIGURG);
}

static int main_loop() {
    // Create new server socket
    auto server = open_server_socket();
    while (true) {
        auto clientSock = accept(server, nullptr, nullptr);
        if (clientSock < 0) {
            if (errno == EINTR && stop_requested) {
                std::cerr << "got SIGINT, stop working" << std::endl;
            } else {
                std::cerr << "error accepting client, stop working" << std::endl;
            }
            break;
        }

        mi::WorkerGlobal->StartNewSession(clientSock);
    }

    shutdown(server, SHUT_RDWR);
    close(server);
    return 0;
}

static void processArguments(int argc, char **argv) {
    for (auto i = 1; i < argc; ++i) {
        auto arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            std::cout << "--help\tshow this help message\n"
                      << "-D [WORKDIR]\tset working directory" << std::endl;
            exit(0);
        }

        if (strcmp(arg, "-D") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "work dir is not provided" << std::endl;
                exit(1);
            }

            i++;
            auto dir = argv[i];
            auto buf = std::array<char, 128>{};
            getcwd(buf.data(), buf.size());
            if (chdir(dir) < 0) {
                std::cerr << "could not chdir to " << dir << ": " << strerror(errno) << std::endl;
                exit(1);
            }

            continue;
        }

        std::cerr << "Unknown option " << arg << std::endl;
        exit(1);
    }
}

int main(int argc, char **argv) {
    processArguments(argc, argv);

    // Register simple handler to stop processing
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::cerr << "could not setup sigint handler" << std::endl;
        return 1;
    }

    // Create new server socket
    auto ret = main_loop();
    if (ret != 0) {
        std::cerr << "invalid return code " << ret << std::endl;
    }

    return ret;
}

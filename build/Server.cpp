#include "Server.h"


Server::Server(uint16_t port) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }
}

Server::~Server() {
    close(fd);
}

void Server::run() {
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        handleConnection(connfd);
        close(connfd);
    }
}

void Server::die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void Server::handleConnection(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

void Server::msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

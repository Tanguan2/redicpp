#include "Server.h"


Server::Server(uint16_t port) : running(true) {
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
    while (running) {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }
        // // handle multiple requests
        while (true) {
            int32_t err = oneRequest(connfd);
            if (err) {
                break;
            }
        }
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

int32_t Server::read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1; // error
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

int32_t Server::write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1; // error
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t maxMsgLen = 8192;

int32_t Server::oneRequest(int connfd) {
    // 4 bytes header
    char rbuf[4 + maxMsgLen + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > maxMsgLen) {
        msg("message too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    
    // BALL
    rbuf[4 + len] = '\0';
    std::cout << "client says: " << &rbuf[4] << std::endl;
    // REPLY
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4); // assume little endian
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

void Server::stop() {
    std::cout << "stopping" << std::endl;
    running = false;
    close(fd);
}

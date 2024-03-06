#include "Client.h"


Client::Client(uint16_t port, const char* ip_address) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_address);

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }
}

Client::~Client() {
    close(fd);
}

void Client::sendMessage(const char* msg) {
    write(fd, msg, strlen(msg));
}

void Client::receiveMessage(char* buffer, size_t buffer_size) {
    ssize_t n = read(fd, buffer, buffer_size - 1);
    if (n < 0) {
        die("read");
    }
    buffer[n] = '\0';
}

void Client::die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

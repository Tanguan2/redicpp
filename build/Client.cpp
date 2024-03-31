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

void Client::msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

int32_t Client::read_full(int fd, char *buf, size_t n) {
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

int Client::getFd() const {
    return fd;
}

int32_t Client::write_all(int fd, const char *buf, size_t n) {
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

int32_t Client::query(int fd, const char *txt) {
    uint32_t len = (uint32_t)strlen(txt);
    if (len > maxMsgLen) {
        return -1;
    }
    char wbuf[4 + maxMsgLen];
    memcpy(wbuf, &len, 4); // copy length
    memcpy(&wbuf[4], txt, len); // copy message
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }
    // 4 byte header
    char rbuf[4 + maxMsgLen + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    memcpy(&len, rbuf, 4); // assume little endian
    if (len > maxMsgLen) {
        msg("message too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // ball
    rbuf[4 + len] = '\0';
    std::cout << "Received: " << rbuf + 4 << std::endl;
}

void Client::closeConnection() {
    close(fd);
}

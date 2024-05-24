#include "Server.h"

// Server class constructor, sets up the server socket
Server::Server(uint16_t port) : running(true) {
    // Create a socket file descriptor
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()"); // Handle socket creation error
    }

    // Set socket option to reuse address
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Set up server address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified address and port
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()"); // Handle bind error
    }

    // Listen for incoming connections
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()"); // Handle listen error
    }
}

// Server class destructor, closes the socket file descriptor
Server::~Server() {
    close(fd);
}

// Run the server, accepting and handling incoming connections
int Server::run() {
    while (running) {
        std::vector<Conn *> fd2conn;
        fd_set_nb(fd);
        std::vector<struct pollfd> pollArgs;
        while (true) {
            pollArgs.clear();
            struct pollfd pfd = {fd, POLLIN, 0};
            pollArgs.push_back(pfd);
            for (Conn *conn: fd2conn) {
                if (!conn) {
                    continue;
                }
                struct pollfd pfd = {};
                pfd.fd = conn->fd;
                pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
                pfd.events |= POLLERR;
                pollArgs.push_back(pfd);
            }
            int rv = poll(pollArgs.data(), (nfds_t)pollArgs.size(), 10000);
            if (rv < 0) {
                die("poll()");
            }
            for (size_t i = 1; i < pollArgs.size(); i++) {
                if (pollArgs[i].revents) {
                    Conn *conn = fd2conn[pollArgs[i].fd];
                    connectionIO(conn);
                    if (conn->state == STATE_DONE) {
                        fd2conn[conn->fd] = NULL;
                        (void)close(conn->fd);
                        delete conn;
                    }
                }
            }

            if (pollArgs[0].revents) {
                (void)acceptNewConn(fd2conn, fd);
            }
        }
    }
    return 0;
}

// Error handling function, prints error message and exits
void Server::die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Print a message to stderr
void Server::msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Read data from a file descriptor until all data is read or an error occurs
int32_t Server::read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1; // Error handling
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Write data to a file descriptor until all data is written or an error occurs
int32_t Server::write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1; // Error handling
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Maximum message length
const size_t maxMsgLen = 4096;

void Server::fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl(F_GETFL)");
        return;
    }
    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl(F_SETFL)");
    }
}

void Server::connPut(std::vector<Conn*> &fd2conn, struct Conn *conn) {
    if (fd2conn.size() <= (size_t)conn->fd) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}

std::mutex Server::accept_mutex;

int32_t Server::acceptNewConn(std::vector<Conn*> &fd2conn, int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    std::lock_guard<std::mutex> guard(accept_mutex);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        // std::cerr << "accept() failed with errno " << errno << " (" << strerror(errno) << ")" << std::endl;
        msg("accept() error");
        return -1;
    }
    fd_set_nb(connfd);
    struct Conn *conn = new Conn();
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    connPut(fd2conn, conn);
    return 0;
}

bool Server::tryOneRequest(Conn *conn) {
    if (conn->rbuf_size < 4) {
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > maxMsgLen) {
        msg("message too long");
        conn->state = STATE_DONE;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        return false;
    }
    std::cout << "Client says: " << conn->rbuf + 4 << std::endl;
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    size_t rem = conn->rbuf_size - (4 + len);
    if (rem) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], rem);
    }
    conn->rbuf_size = rem;
    stateResponse(conn);
    return (conn->state == STATE_REQ);
}

bool Server::tryFillRbuf(Conn *conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t c = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], c);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_DONE;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size == 0) {
            msg("Unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_DONE;
        return false;
    }
    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));
    while (tryOneRequest(conn)) {}
    return (conn->state == STATE_REQ);
}

void Server::stateRequest(Conn *conn) {
    while (tryFillRbuf(conn)) {}
}

bool Server::tryFlushWbuf(Conn *conn) {
    ssize_t rv = 0;
    do {
        size_t rem = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], rem);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_DONE;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->state = STATE_REQ;
        conn->wbuf_size = 0;
        conn->wbuf_sent = 0;
        return false;
    }
    return true;
}

void Server::stateResponse(Conn *conn) {
    while (tryFlushWbuf(conn)) {}
}

void Server::connectionIO(Conn *conn) {
    if (conn->state == STATE_REQ) {
        stateRequest(conn);
    } else if (conn->state == STATE_RESP) {
        stateResponse(conn);
    } else {
        assert(0);
    }
}

// Stop the server
void Server::stop() {
    std::cout << "stopping" << std::endl;
    running = false;
    close(fd);
}
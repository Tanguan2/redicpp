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
    int kq = kqueue();
    if (kq == -1) {
        // std::cerr << "Failed to create kqueue\n";
        die("kqueue()");
    }
    // std::cerr << "kqueue created successfully\n";

    struct kevent changeList[2];
    EV_SET(&changeList[0], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kq, changeList, 1, NULL, 0, NULL) == -1) {
        // std::cerr << "Failed to add listening socket to kqueue\n";
        die("kevent()");
    }
    // std::cerr << "Listening socket added to kqueue\n";

    std::vector<Conn *> fd2conn;
    fd_set_nb(fd);
    // std::cerr << "Server is now non-blocking and ready to accept connections\n";

    while (running) {
        struct kevent eventList[32];
        int numEvents = kevent(kq, NULL, 0, eventList, 32, NULL);
        if (numEvents == -1) {
            // std::cerr << "Error during kevent wait\n";
            die("kevent()");
        }
        // std::cerr << "Processed " << numEvents << " events\n";

        for (int i = 0; i < numEvents; ++i) {
            int eventFd = eventList[i].ident;
            int eventFilter = eventList[i].filter;

            if (eventFd == fd && eventFilter == EVFILT_READ) {
                // std::cerr << "Accepting new connection\n";
                acceptNewConn(fd2conn, fd, kq);
            } else {
                Conn *conn = fd2conn[eventFd];
                if (conn) {
                    // std::cerr << "Processing connection on fd " << eventFd << "\n";
                    if (eventFilter == EVFILT_READ || eventFilter == EVFILT_WRITE) {
                        connectionIO(conn);
                        if (conn->state == STATE_DONE) {
                            // std::cerr << "Connection on fd " << conn->fd << " is done, closing\n";
                            fd2conn[conn->fd] = NULL;
                            EV_SET(&changeList[0], conn->fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                            EV_SET(&changeList[1], conn->fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
                            kevent(kq, changeList, 2, NULL, 0, NULL);
                            (void)close(conn->fd);
                            delete conn;
                        } else if (conn->state == STATE_REQ) {
                            EV_SET(&changeList[0], conn->fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                            kevent(kq, changeList, 1, NULL, 0, NULL);
                        } else if (conn->state == STATE_RESP) {
                            EV_SET(&changeList[0], conn->fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
                            kevent(kq, changeList, 1, NULL, 0, NULL);
                        }
                    }
                } else {
                    // std::cerr << "No connection object found for fd " << eventFd << "\n";
                }
            }
        }
    }
    // std::cerr << "Server shutting down, closing kqueue\n";
    (void)close(kq);
    return 0;
}


// Error handling function, prints error message and exits
void Server::die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Handle a client connection, read and write data
void Server::handleConnection(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error"); // Handle read error
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
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

// Handle a single client request
int32_t Server::oneRequest(int connfd) {
    // Read the request header (4 bytes)
    char rbuf[4 + maxMsgLen + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF"); // Handle EOF
        } else {
            msg("read() error"); // Handle read error
        }
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // Assume little endian
    if (len > maxMsgLen) {
        msg("message too long"); // Handle message length error
        return -1;
    }

    // Read the request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error"); // Handle read error
        return err;
    }
    
    // Print the client
        // Null-terminate the received data
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

int32_t Server::acceptNewConn(std::vector<Conn*> &fd2conn, int fd, int kq) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    std::lock_guard<std::mutex> guard(accept_mutex);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        std::cerr << "accept() failed with errno " << errno << " (" << strerror(errno) << ")" << std::endl;
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

    struct kevent changeList[2];
    EV_SET(&changeList[0], connfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    EV_SET(&changeList[1], connfd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    kevent(kq, changeList, 2, NULL, 0, NULL);

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
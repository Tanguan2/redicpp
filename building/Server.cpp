#include "Server.h"

Server::Server(uint16_t port) : running(true) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }
}

Server::~Server() {
    close(fd);
}

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

void Server::msg(const char *msg) {
    fprintf(stderr, "Server: %s\n", msg);
}

void Server::die(const char *msg) {
    int err = errno;
    fprintf(stderr, "Server: [%d] %s\n", err, msg);
    abort();
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

const size_t k_max_msg = 4096;

void Server::connPut(std::vector<Conn*> &fd2conn, struct Conn *conn) {
    fd2conn[conn->fd] = conn;
}

int32_t Server::acceptNewConn(std::vector<Conn*> &fd2conn, int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        // msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
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

const size_t k_max_args = 1024;

int32_t Server::parseReq(const uint8_t *data, size_t len, std::vector<std::string> &out) {
    if (len < 4) {
        return -1;
    }
    uint32_t n = 0;
    memcpy(&n, &data[0], 4);
    if (n > k_max_args) {
        return -1;
    }

    size_t pos = 4;
    while (n--) {
        if (pos + 4 > len) {
            return -1;
        }
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);
        if (pos + 4 + sz > len) {
            return -1;
        }
        out.push_back(std::string((char *)&data[pos + 4], sz));
        pos += 4 + sz;
    }

    if (pos != len) {
        return -1;  // trailing garbage
    }
    return 0;
}

static std::unordered_map<std::string, std::string> g_map;

uint32_t Server::do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
    if (!g_map.count(cmd[1])) {
        return RES_NX;
    }
    std::string &val = g_map[cmd[1]];
    assert(val.size() <= k_max_msg);
    memcpy(res, val.data(), val.size());
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

uint32_t Server::do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
    (void)res;
    (void)reslen;
    g_map[cmd[1]] = cmd[2];
    return RES_OK;
}

uint32_t Server::do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
    (void)res;
    (void)reslen;
    g_map.erase(cmd[1]);
    return RES_OK;
}

bool Server::cmd_is(const std::string &word, const char *cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

int32_t Server::do_request(const uint8_t *req, uint32_t reqlen, uint32_t *rescode, uint8_t *res, uint32_t *reslen) {
    std::vector<std::string> cmd;
    if (0 != parseReq(req, reqlen, cmd)) {
        msg("bad req");
        return -1;
    }
    if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        *rescode = do_get(cmd, res, reslen);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        *rescode = do_set(cmd, res, reslen);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        *rescode = do_del(cmd, res, reslen);
    } else {
        // cmd is not recognized
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char *)res, msg);
        *reslen = strlen(msg);
        return 0;
    }
    return 0;
}

bool Server::tryOneRequest(Conn *conn) {
    // try to parse a request from the buffer
    if (conn->rbuf_size < 4) {
        // not enough data in the buffer. Will retry in the next iteration
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_DONE;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        // not enough data in the buffer. Will retry in the next iteration
        return false;
    }

    // got one request, generate the response.
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(
        &conn->rbuf[4], len,
        &rescode, &conn->wbuf[4 + 4], &wlen
    );
    if (err) {
        conn->state = STATE_DONE;
        return false;
    }
    wlen += 4;
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    conn->wbuf_size = 4 + wlen;

    // remove the request from the buffer.
    // note: frequent memmove is inefficient.
    // note: need better handling for production code.
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    // change state
    conn->state = STATE_RESP;
    stateResponse(conn);

    // continue the outer loop if the request was fully processed
    return (conn->state == STATE_REQ);
}

bool Server::tryFillRbuf(Conn *conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_DONE;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_DONE;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // Try to process requests one by one.
    // Why is there a loop? Please read the explanation of "pipelining".
    while (tryOneRequest(conn)) {}
    return (conn->state == STATE_REQ);
}

void Server::stateRequest(Conn *conn) {
    while (tryFillRbuf(conn)) {}
}

bool Server::tryFlushWbuf(Conn *conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
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
        // response was fully sent, change state back
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    // still got some data in wbuf, could try to write again
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
        assert(0);  // not expected
    }
}
#ifndef SERVER_H
#define SERVER_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <cstring>
#include <cerrno>
#include <sys/event.h>
#include <sys/time.h>
#include <fstream>
#include <chrono>
#include <ctime>

enum {
    STATE_REQ = 0,
    STATE_RESP = 1,
    STATE_DONE = 2,
};

struct Conn {
    int fd = -1;
    uint32_t state = 0;
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + 4096];
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + 4096];
};

class Server {
public:
    Server(uint16_t port);
    ~Server();
    int run();
    void stop();
    static void connPut(std::vector<Conn*> &fd2conn, struct Conn *conn);
    static int32_t acceptNewConn(std::vector<Conn*> &fd2conn, int fd, int kq);
    static void stateRequest(Conn *conn);
    static void stateResponse(Conn *conn);
    static bool tryOneRequest(Conn *conn);
    static bool tryFillRbuf(Conn *conn);
    static bool tryFlushWbuf(Conn *conn);
    static void connectionIO(Conn *conn);
    void logRequest(const Conn *conn, const std::string &clientMsg);

private:
    int fd;
    bool running;
    static void die(const char *msg);
    static void msg(const char *msg);
    static int32_t read_full(int fd, char *buf, size_t n);
    static int32_t write_all(int fd, const char *buf, size_t n);
    static void fd_set_nb(int fd);
    static std::mutex accept_mutex;
    static std::mutex log_mutex;
    static std::ofstream logfile;
};

#endif // SERVER_H

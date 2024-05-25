#ifndef SERVER_H
#define SERVER_H

#include "Dependencies.h"

enum {
    STATE_REQ = 0,
    STATE_RESP = 1,
    STATE_DONE = 2,
};

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
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
    static int32_t acceptNewConn(std::vector<Conn*> &fd2conn, int fd);
    static void stateRequest(Conn *conn);
    static void stateResponse(Conn *conn);
    static bool tryOneRequest(Conn *conn);
    static bool tryFillRbuf(Conn *conn);
    static bool tryFlushWbuf(Conn *conn);
    static void connectionIO(Conn *conn);
    void logRequest(const Conn *conn, const std::string &clientMsg);
    static void die(const char *msg);
    static void msg(const char *msg);
    static int32_t read_full(int fd, char *buf, size_t n);
    static int32_t write_all(int fd, const char *buf, size_t n);
    static void fd_set_nb(int fd);
    static int32_t parseReq(const uint8_t *data, size_t len, std::vector<std::string> &out);
    static uint32_t do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
    static uint32_t do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
    static uint32_t do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
    static bool cmd_is(const std::string &word, const char *cmd);
    static int32_t do_request(const uint8_t *req, uint32_t reqlen,uint32_t *rescode, uint8_t *res, uint32_t *reslen);

private:
    int fd;
    bool running;
    static std::mutex accept_mutex;
    static std::mutex log_mutex;
    static std::ofstream logfile;
};

#endif
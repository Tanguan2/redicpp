#ifndef CLIENT_H
#define CLIENT_H

#include "Dependencies.h"

// CLIENT_H

class Client {
public:
    Client(uint16_t port, const char* ip_address);
    ~Client();
    int getFd() const;
    void closeConnection();
    static int32_t sendRequest(int fd, const std::vector<std::string> &cmd);
    static int32_t readRequest(int fd);
    static void die(const char *msg);
    static void msg(const char *msg);
    static int32_t read_full(int fd, char *buf, size_t n);
    static int32_t write_all(int fd, const char *buf, size_t n);

private:
    int fd;
};


#endif
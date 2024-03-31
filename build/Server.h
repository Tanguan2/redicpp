#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>

class Server {
public:
    Server(uint16_t port);
    ~Server();
    void run();
    void stop();

private:
    int fd;
    bool running;
    static void die(const char *msg);
    static void handleConnection(int connfd);
    static void msg(const char *msg);
    static int32_t read_full(int fd, char *buf, size_t n);
    static int32_t write_all(int fd, const char *buf, size_t n);
    static int32_t oneRequest(int connfd);
};

#endif // SERVER_H

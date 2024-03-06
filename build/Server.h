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

class Server {
public:
    Server(uint16_t port);
    ~Server();
    void run();

private:
    int fd;
    static void die(const char *msg);
    static void handleConnection(int connfd);
    static void msg(const char *msg);
};

#endif // SERVER_H

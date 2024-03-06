#ifndef CLIENT_H
#define CLIENT_H

#include <cstddef> // for size_t
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

class Client {
public:
    Client(uint16_t port, const char* ip_address);
    ~Client();
    void sendMessage(const char* msg);
    void receiveMessage(char* buffer, size_t buffer_size);

private:
    int fd;
    static void die(const char *msg);
};

#endif // CLIENT_H

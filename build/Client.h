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
#include <iostream>

class Client {
public:
    Client(uint16_t port, const char* ip_address);
    ~Client();
    int getFd() const;
    void sendMessage(const char* msg);
    void receiveMessage(char* buffer, size_t buffer_size);
    void closeConnection();
    static int32_t query(int fd, const char *txt);

private:
    int fd;
    static void die(const char *msg);
    static void msg(const char *msg);
    static int32_t read_full(int fd, char *buf, size_t n);
    static int32_t write_all(int fd, const char *buf, size_t n);
};

#endif // CLIENT_H

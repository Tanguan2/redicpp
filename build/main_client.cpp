#include "Client.h"

int main() {
    Client client(1234, "127.0.0.1");

    char msg[] = "msg1test";
    client.sendMessage(msg);

    char rbuf[64] = {};
    client.receiveMessage(rbuf, sizeof(rbuf));
    printf("server says: %s\n", rbuf);

    return 0;
}
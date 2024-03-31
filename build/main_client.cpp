#include "Client.h"

int main() {
    Client client(1234, "127.0.0.1");
    const char* msg = "msg1test";
    int32_t err = client.query(client.getFd(), msg);
    if (err) {
        printf("Error sending query\n");
    }
    return 0;
}
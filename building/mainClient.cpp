#include "Client.h"

int main(int argc, char **argv) {
    Client client(1234, "127.0.0.1");
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) {
        cmd.push_back(argv[i]);
    }
    int32_t err = client.sendRequest(client.getFd(), cmd);
    if (err) {
        printf("Error sending query\n");
        close(client.getFd());
        return 1;
    }
    err = client.readRequest(client.getFd());
    if (err) {
        printf("Error reading response\n");
        close(client.getFd());
        return 1;
    }
}
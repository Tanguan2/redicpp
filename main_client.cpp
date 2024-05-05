#include "Client.h"

int main(int argc, char **argv) {
    Client client(1234, "127.0.0.1");
    // const char* msgs[3] = {"hello1", "hello2", "hello3"};
    // for (size_t i = 0; i < 3; i++){
    //     int32_t err = client.sendRequest(client.getFd(), msgs[i]);
    //     if (err) {
    //         printf("Error sending query\n");
    //     }   
    // }
    // for (size_t i = 0; i < 3; i++){
    //     int32_t err = client.readRequest(client.getFd());
    //     if (err) {
    //         printf("Error sending query\n");
    //     }   
    // }
    std::vector<std::string> tokens;
    for (int i = 1; i < argc; i++) {
        tokens.push_back(argv[i]);
    }
    int32_t err = client.sendRequest(client.getFd(), tokens);
    if (err) {
        client.closeConnection();
        return 0;
    }
    err = client.readRequest(client.getFd());
    if (err) {
        client.closeConnection();
        return 0;
    }
    client.closeConnection();
    return 0;
}
#include "Client.h"

int main() {
    Client client(1234, "127.0.0.1");
    const char* msgs[3] = {"hello1", "hello2", "hello3"};
    for (size_t i = 0; i < 3; i++){
        int32_t err = client.sendRequest(client.getFd(), msgs[i]);
        if (err) {
            printf("Error sending query\n");
        }   
    }
    for (size_t i = 0; i < 3; i++){
        int32_t err = client.readRequest(client.getFd());
        if (err) {
            printf("Error sending query\n");
        }   
    }
    return 0;
}
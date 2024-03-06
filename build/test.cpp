#include <iostream>
#include <thread>
#include "Client.h"
#include "Server.h"

void startServer() {
    Server server(1234);
    server.run();
}

void testClientServerCommunication() {
    std::thread serverThread(startServer);

    // Give the server some time to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    Client client1(1234, "127.0.0.1");
    Client client2(1234, "127.0.0.1");
    client2.sendMessage("Hello, server from client 2!");
    client1.sendMessage("Hello, server from client 1!");

    char response[64] = {};
    std::cout << "Client 2 Receiving Message" << std::endl;
    client2.receiveMessage(response, sizeof(response));
    std::cout << "Client 1 Receiving Message" << std::endl;
    client1.receiveMessage(response, sizeof(response));

    if (strcmp(response, "world") == 0) {
        std::cout << "Test passed: Received expected response from server." << std::endl;
    } else {
        std::cout << "Test failed: Unexpected response from server." << std::endl;
    }

    // Stop the server thread
    serverThread.detach();
}

int main() {
    testClientServerCommunication();
    return 0;
}

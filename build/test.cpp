#include <iostream>
#include <thread>
#include "Client.h"
#include "Server.h"
#include <gtest/gtest.h>

class ClientServerTest : public ::testing::Test {
protected:
    std::thread serverThread;

    void SetUp() override {
        // Start the server in a separate thread
        serverThread = std::thread([] {
            Server server(1234);
            server.run();
        });

        // Give the server some time to start
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        // Stop the server thread
        serverThread.detach();
    }
};

TEST_F(ClientServerTest, ClientServerCommunication) {
    Client client1(1234, "127.0.0.1");
    Client client2(1234, "127.0.0.1");

    client2.sendMessage("Hello, server from client 2!");
    client1.sendMessage("Hello, server from client 1!");

    char response1[64] = {};
    char response2[64] = {};

    std::cout << "Client 2 Receiving Message" << std::endl;
    client2.receiveMessage(response2, sizeof(response2));

    std::cout << "Client 1 Receiving Message" << std::endl;
    client1.receiveMessage(response1, sizeof(response1));

    EXPECT_STREQ(response2, "world");
    EXPECT_STREQ(response1, "world");
}

TEST_F(ClientServerTest, MultipleMessages) {
    Client client(1234, "127.0.0.1");

    const int NUM_MESSAGES = 5;
    const char* messages[NUM_MESSAGES] = {
        "Message 1",
        "Message 2",
        "Message 3",
        "Message 4",
        "Message 5"
    };

    char response[64] = {};

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        client.sendMessage(messages[i]);

        // Assuming the server echoes the message back
        client.receiveMessage(response, sizeof(response));
        std::cout << "Received: " << response << std::endl;
        EXPECT_STREQ(response, messages[i]) << "Failed at message index " << i;
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

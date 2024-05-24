#include "Client.h"
#include "Server.h"
#include <gtest/gtest.h>

class ClientServerTest : public ::testing::Test {
protected:
    std::thread serverThread;
    Server server{1234};
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    void SetUp() override {
        // Start the server in a separate thread
        serverThread = std::thread([this] {
            server.run();
        });

        // Give the server some time to start
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        // Stop the server thread
        server.stop();
        serverThread.detach();
    }
};

TEST_F(ClientServerTest, ClientServerCommunication) {
    Client client1(1234, "127.0.0.1");
    Client client2(1234, "127.0.0.1");
    Client client3(1234, "127.0.0.1");

    // Use the query method to send messages and receive responses
    std::string message1 = "hello1";
    std::string message2 = "hello2";
    std::string message3 = "hello3";

    int32_t result1 = client1.sendRequest(client1.getFd(), message1.c_str());
    result1 = client1.readRequest(client1.getFd());
    int32_t result2 = client2.sendRequest(client2.getFd(), message2.c_str());
    result2 = client2.readRequest(client2.getFd());
    int32_t result3 = client3.sendRequest(client3.getFd(), message3.c_str());
    result3 = client3.readRequest(client3.getFd());

    // Check if the queries were successful
    EXPECT_EQ(result1, 0) << "Query failed with error code " << result1;
    EXPECT_EQ(result2, 0) << "Query failed with error code " << result2;
    EXPECT_EQ(result3, 0) << "Query failed with error code " << result3;

    // The server's responses will be printed to the console by the query method
    // Since the server responds with "world", we don't need to explicitly check it here
}

// concurrency test
TEST_F(ClientServerTest, MultipleConcurrentClients) {
    const int num_clients = 5;
    std::vector<std::thread> client_threads;
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back([i] {
            Client client(1234, "127.0.0.1");
            std::string message = "hello" + std::to_string(i);
            int32_t result = client.sendRequest(client.getFd(), message.c_str());
            EXPECT_EQ(result, 0) << "Query failed with error code " << result;
            result = client.readRequest(client.getFd());
            EXPECT_EQ(result, 0) << "Query failed with error code " << result;
        });
    }
    for (auto& t : client_threads) {
        t.join();
    }
}

// large message test
TEST_F(ClientServerTest, LargeMessage) {
    Client client(1234, "127.0.0.1");
    std::string large_message(4096, 'A'); // Create a large message
    int32_t result = client.sendRequest(client.getFd(), large_message.c_str());
    EXPECT_EQ(result, 0) << "Query failed with error code " << result;
    result = client.readRequest(client.getFd());
    EXPECT_EQ(result, 0) << "Query failed with error code " << result;
}

// invalid message length test
TEST_F(ClientServerTest, InvalidMessageLength) {
    Client client(1234, "127.0.0.1");
    std::string long_message(4097, 'B'); // Create a message longer than maxMsgLen
    int32_t result = client.sendRequest(client.getFd(), long_message.c_str());
    EXPECT_NE(result, 0) << "Server should reject messages longer than maxMsgLen";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

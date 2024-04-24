CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -g -std=c++14
LDFLAGS =
GTEST_FLAGS = -lgtest -lgtest_main -pthread

# Object files
CLIENT_OBJS = Client.o
SERVER_OBJS = Server.o
TEST_OBJS = test.o

# Executables
CLIENT_EXE = client
SERVER_EXE = server
TEST_EXE = test

.PHONY: all clean test

all: $(CLIENT_EXE) $(SERVER_EXE)

$(CLIENT_EXE): $(CLIENT_OBJS) main_client.o
	$(CXX) $(LDFLAGS) -o $@ $^

$(SERVER_EXE): $(SERVER_OBJS) main_server.o
	$(CXX) $(LDFLAGS) -o $@ $^

test: $(TEST_EXE)

$(TEST_EXE): $(TEST_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(GTEST_FLAGS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

main_client.o: main_client.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main_server.o: main_server.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_EXE) $(SERVER_EXE) $(TEST_EXE) *.o
	rm test.out && rm server.log

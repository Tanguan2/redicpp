# Compiler
CC := g++

# Compiler flags
CFLAGS := -std=c++20 -Wall

# Source files
CLIENT_SRCS := mainClient.cpp Client.cpp
SERVER_SRCS := mainServer.cpp Server.cpp

# Object files
CLIENT_OBJS := $(CLIENT_SRCS:.cpp=.o)
SERVER_OBJS := $(SERVER_SRCS:.cpp=.o)

# Executables
CLIENT_EXEC := client
SERVER_EXEC := server

# Build rules
all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC) $(CLIENT_OBJS) $(SERVER_OBJS)
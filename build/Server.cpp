#include "Server.h"

// Server class constructor, sets up the server socket
Server::Server(uint16_t port) : running(true) {
    // Create a socket file descriptor
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()"); // Handle socket creation error
    }

    // Set socket option to reuse address
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Set up server address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified address and port
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()"); // Handle bind error
    }

    // Listen for incoming connections
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()"); // Handle listen error
    }
}

// Server class destructor, closes the socket file descriptor
Server::~Server() {
    close(fd);
}

// Run the server, accepting and handling incoming connections
void Server::run() {
    while (running) {
        // Accept an incoming connection
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue; // Error handling
        }
        // Handle multiple requests from the same client
        while (true) {
            int32_t err = oneRequest(connfd);
            if (err) {
                break;
            }
        }
        // Close the client connection
        close(connfd);
    }
}

// Error handling function, prints error message and exits
void Server::die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Handle a client connection, read and write data
void Server::handleConnection(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error"); // Handle read error
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

// Print a message to stderr
void Server::msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Read data from a file descriptor until all data is read or an error occurs
int32_t Server::read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1; // Error handling
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Write data to a file descriptor until all data is written or an error occurs
int32_t Server::write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1; // Error handling
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Maximum message length
const size_t maxMsgLen = 4096;

// Handle a single client request
int32_t Server::oneRequest(int connfd) {
    // Read the request header (4 bytes)
    char rbuf[4 + maxMsgLen + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF"); // Handle EOF
        } else {
            msg("read() error"); // Handle read error
        }
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // Assume little endian
    if (len > maxMsgLen) {
        msg("message too long"); // Handle message length error
        return -1;
    }

    // Read the request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error"); // Handle read error
        return err;
    }
    
    // Print the client
        // Null-terminate the received data
    rbuf[4 + len] = '\0';
    std::cout << "client says: " << &rbuf[4] << std::endl;
    // REPLY
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4); // assume little endian
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

// Stop the server
void Server::stop() {
    std::cout << "stopping" << std::endl;
    running = false;
    close(fd);
}
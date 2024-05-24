#include "Client.h"


// Constructor for the Client class, takes a port number and IP address as parameters
Client::Client(uint16_t port, const char* ip_address) {
    // Create a socket file descriptor using the AF_INET address family (IPv4) and SOCK_STREAM socket type (TCP)
    fd = socket(AF_INET, SOCK_STREAM, 0);
    // If the socket creation fails, call the die function with the error message "socket()"
    if (fd < 0) {
        die("socket()");
    }

    // Initialize a sockaddr_in structure to store the server address
    struct sockaddr_in addr = {};
    // Set the address family to AF_INET (IPv4)
    addr.sin_family = AF_INET;
    // Set the port number to the given port, converting it to network byte order using htons
    addr.sin_port = htons(port);
    // Set the IP address to the given ip_address, converting it to a binary format using inet_addr
    addr.sin_addr.s_addr = inet_addr(ip_address);

    // Attempt to connect to the server using the socket file descriptor and the server address
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    // If the connection fails, call the die function with the error message "connect"
    if (rv) {
        die("connect");
    }
}

// Destructor for the Client class
Client::~Client() {
    // Close the socket file descriptor to release system resources
    close(fd);
}

// Error handling function to print an error message and exit the program
void Client::die(const char *msg) {
    // Save the current errno value
    int err = errno;
    // Print the error message to stderr, including the errno value and the given message
    fprintf(stderr, "[%d] %s\n", err, msg);
    // Abort the program immediately, terminating the process
    abort();
}

// Function to print a message to stderr
void Client::msg(const char *msg) {
    // Print the given message to stderr
    fprintf(stderr, "%s\n", msg);
}

// Read data from a file descriptor until the requested amount is read or an error occurs
int32_t Client::read_full(int fd, char *buf, size_t n) {
    // Loop until all data is read
    while (n > 0) {
        // Read data from the file descriptor into the buffer
        ssize_t rv = read(fd, buf, n);
        // If the read operation fails or reaches end-of-file, return an error
        if (rv <= 0) {
            return -1; // error
        }
        // Update the remaining bytes to read and the buffer pointer
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    // Return success
    return 0;
}

// Getter function to retrieve the file descriptor associated with the client
int Client::getFd() const {
    // Return the file descriptor
    return fd;
}

// Write data to a file descriptor until all data is written or an error occurs
int32_t Client::write_all(int fd, const char *buf, size_t n) {
    // Loop until all data is written
    while (n > 0) {
        // Write data from the buffer to the file descriptor
        ssize_t rv = write(fd, buf, n);
        // If the write operation fails or writes no data, return an error
        if (rv <= 0) {
            return -1; // error
        }
        // Update the remaining bytes to write and the buffer pointer
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    // Return success
    return 0;
}

// Maximum message length
const size_t maxMsgLen = 4096;

int32_t Client::sendRequest(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = 4;
    for (const std::string &s: cmd) {
        len += 4 + s.size();
    }
    if (len > maxMsgLen) {
        msg("message too long");
        return -1;
    }
    char wbuf[4 + maxMsgLen];
    memcpy(&wbuf[0], &len, 4);  // assume little endian
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
    return write_all(fd, wbuf, 4 + len);
}

int32_t Client::readRequest(int fd) {
    char rbuf[4 + maxMsgLen + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > maxMsgLen) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
    return 0;
}

// Close the client connection
void Client::closeConnection() {
    // Close the file descriptor
    close(fd);
}

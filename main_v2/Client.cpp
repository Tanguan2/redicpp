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

int32_t Client::sendRequest(int fd, const char *txt) {
    // Get the length of the request text
    uint32_t len = (uint32_t)strlen(txt);
    // Check if the request is too long
    if (len > maxMsgLen) {
        return -1;
    }
    // Create a write buffer to hold the length and request text
    char wbuf[4 + maxMsgLen];
    // Copy the length to the first 4 bytes of the write buffer
    memcpy(wbuf, &len, 4);
    // Copy the request text to the write buffer after the length
    memcpy(&wbuf[4], txt, len);
    return write_all(fd, wbuf, 4 + len);
}

int32_t Client::readRequest(int fd) {
    // Create a read buffer to hold the request
    char rbuf[4 + maxMsgLen + 1];
    // Reset errno
    errno = 0;
    // Read the request length from the file descriptor
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        // Handle errors
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }
    // Get the request length from the read buffer
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    // Check if the request is too long
    if (len > maxMsgLen) {
        msg("message too long");
        return -1;
    }
    // Read the request body from the file descriptor
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    // Null-terminate the request
    rbuf[4 + len] = '\0';
    // Print the received request
    std::cout << "Received: " << rbuf + 4 << std::endl;
    return 0;
}

// Close the client connection
void Client::closeConnection() {
    // Close the file descriptor
    close(fd);
}

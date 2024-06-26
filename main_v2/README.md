# Redicpp Project

This repository contains the source code for the Redicpp client and server applications, as well as a suite of tests to ensure functionality.

## Prerequisites

For now, this project is developed on MacOS. This is due to how we're handling polling (for MacOS, I have to use kqueue because epoll isn't implemented U_U). I'll move to multi-platform cmake with epoll for Linux users in the future, or you can take it upon yourself to implement this for me and submit a PR.

Before building and running this project, ensure you have the following installed:

- CMake (minimum version 3.10)
- A C++ compiler that supports C++14 (GCC, Clang, etc.)
- Git (for cloning dependencies)

## Getting Started

These instructions will get your copy of the project up and running on your local machine for development and testing purposes.

### Cloning the Repository

To get started, clone this repository to your local machine using:

```bash
git clone https://github.com/yourusername/redicpp.git
cd redicpp

```
Once you have cloned the repository, simply run:
```bash
sh build.sh && cd build
```
Then, you can run the client, server, and test executables as you wish. Additionally, for testing, you can run the following:
- make runClient (for testing the client)
- make runServer (for testing the server)
- make runTests (for running all tests)

Happy coding !
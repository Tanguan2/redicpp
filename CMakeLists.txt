cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Redicpp)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1  # Optionally pin to a specific version/release
)
FetchContent_MakeAvailable(googletest)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories for header files
include_directories(${PROJECT_SOURCE_DIR})

# Add executable for the client
add_executable(client main_client.cpp Client.cpp)

# Add executable for the server
add_executable(server main_server.cpp Server.cpp)

# Add executable for tests
add_executable(tests test.cpp Client.cpp Server.cpp)
target_link_libraries(tests gtest_main gtest)

# custom targets for testing
add_custom_target(runClient
    COMMAND client
    COMMENT "Running the Client application"
)

add_custom_target(runServer
    COMMAND server &
    COMMENT "Running the Server application"
)

# Custom target for running tests
add_custom_target(runTests
    COMMAND tests
    COMMENT "Running the Tests"
)

# Compiler options to ignore deprecated copy warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-deprecated-copy)
endif()

# Enable testing with CTest and Google Test
enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)
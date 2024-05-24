#!/bin/sh

# Run the makefile
make test

# Run the server executable in the background
./test > test.out

# Count occurrences of 'TEST_F' in test.cpp
num_expected_tests=$(grep -c 'TEST_F' test.cpp)
echo "Expected number of tests: $num_expected_tests"

# Extract the number of tests passed from the last relevant line in test.out
last_line=$(grep '\[  PASSED  \]' test.out | tail -n 1)
num_passed_tests=$(echo "$last_line" | grep -oE '[0-9]+')

if [ "$num_passed_tests" = "$num_expected_tests" ]; then
  echo "Test assertion PASSED: Number of passed tests matches the expected count ($num_passed_tests tests)."
else
  echo "Test assertion FAILED: Mismatch in the number of passed tests."
  echo "Expected: $num_expected_tests"
  echo "Found: $num_passed_tests"
  exit 1
fi
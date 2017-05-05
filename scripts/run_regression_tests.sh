#!/bin/bash

echo "Checking out HEAD~1..."
git checkout HEAD~1
echo "Running regression tests..."
make regression_tests
echo "Checking out current commit..."
git checkout -
echo "Cleanup..."
make clean
echo "Running regression tests..."
make regression_tests

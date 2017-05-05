#!/bin/bash

echo "Checking out HEAD~1..."
git checkout HEAD~1
echo "Running tests..."
make delta_tests
echo "Checking out current commit..."
git checkout -
echo "Cleanup..."
make clean
echo "Running tests..."
make delta_tests

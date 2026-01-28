#!/bin/bash

# Compile
echo "Compiling i2c_diag..."
g++ -o i2c_diag i2c_diag.cpp

if [ $? -eq 0 ]; then
    echo "Compilation successful."
    echo "Running i2c_diag..."
    sudo ./i2c_diag
else
    echo "Compilation failed."
fi

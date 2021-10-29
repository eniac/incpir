#!/bin/bash
mkdir -p build
cd build
cmake ..
make -j
echo
echo "db 2^16"
./bin/localbench -d 16
echo
echo "db 2^18"
./bin/localbench -d 18
echo
echo "db 2^20"
./bin/localbench -d 20

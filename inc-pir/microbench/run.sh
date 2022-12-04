#!/bin/bash
echo "db 2^16"
../build/bin/microbench -d 16
echo
echo "db 2^18"
../build/bin/microbench -d 18
echo
echo "db 2^20"
../build/bin/microbench -d 20

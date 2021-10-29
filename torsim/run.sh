#!/bin/bash
mkdir -p build
cd build 
cmake ..
make -j

echo
echo "running..."
./bin/simulate -q 100 -c >> log.txt
./bin/simulate -q 200 -c >> log.txt
./bin/simulate -q 500 -c >> log.txt

./bin/simulate -q 100 >> log.txt

cd ..
python3 graph_server.py >> log.txt
python3 graph_client.py >> log.txt

echo "finished"

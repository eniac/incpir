#!/bin/bash

echo "running..."
../build/bin/simulate -q 100 -c >> log.txt
../build/bin/simulate -q 200 -c >> log.txt
../build/bin/simulate -q 500 -c >> log.txt

../build/bin/simulate -q 100 >> log.txt

python3 graph_server.py >> log.txt
python3 graph_client.py >> log.txt

echo "finished"

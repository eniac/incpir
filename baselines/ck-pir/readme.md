To build, run
```
mkdir -p build
cd build
cmake ..
make -j
```

For microbenchmarks, run `./bin/CK`.

For end-to-end tests, run 
```
./server -o -i 0.0.0.0:6666 -t 10 -g 13 -d 7000
./online_client_eval -i [XXX.XXX.XXX.XXX]:6666 -t 10 -l 100 -g 13 -d 7000
```
where the parameters are
```
-i [ip addr] 
-t [fixed time (0.01 per unit)] -l [offer load] 
-g [upper lg db size] -d [db size] 
```


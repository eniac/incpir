To build, run
```
mkdir -p build  
cd build
cmake ..
make -j
```

In folder `bin`, run `./offline_server_eval` and `./offline_client_eval` on two machines with the following parameters:

```
-i [ip addr] 
-t [fixed time (0.01 per unit)] -l [offer load] 
-d [db size] -s [set size] -n [nbrsets] 
-a [incprep] -b [baseline]
```

where parameters for the server and the client should be the same except ip. For example,

Prep

```sh
./offline_server_eval -i 0.0.0.0:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020 -b
./offline_client_eval -i [XXX.XXX.XXX.XXX]:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020 -b
```

IncPrep

```sh
./offline_server_eval -i 0.0.0.0:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020  -a 70
./offline_client_eval -i [XXX.XXX.XXX.XXX]:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020 -a 70
```

The server will print process time for each task and the client will print latency for each task. 


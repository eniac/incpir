#include <iostream>
#include <cmath>
#include <vector>
#include <tuple>
#include <random>
#include <chrono>

#include <openssl/rand.h>
#include <openssl/evp.h>

#include <set>

#include "src/pir.hpp"
#include "src/puncprf.hpp"
#include "src/client.hpp"
#include "src/server.hpp"
#include "src/serial.hpp"
#include "unistd.h"

using namespace std;

int main(int argc, char* argv[]) {

    // set parms
    uint32_t lgn = 17; // database n = 2^lgn

    uint32_t db_size = 70000;

    int opt;
    while ((opt = getopt(argc, argv, "d:g:")) != -1) {
        if (opt == 'g') {
            lgn = atoi(optarg);
        }
        else if (opt == 'd') {
            db_size = atoi(optarg);
        }
    }
    std::random_device rd;
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

    // setup server
    PIRServer server;
    server.set_database(db_size, &my_database);
    cout << "server database set." << endl;

    // setup client
    PIRClient client;
    client.set_parms(db_size, (1<<(lgn/2)), (1<<(lgn/2))*12);
    client.generate_setkeys();


    // client generates offline query
    OfflineQuery offline_qry = client.generate_offline_query();
    OfflineQuery offline_qry1 = deserialize_offline_query(serialize_offline_query(offline_qry));

    cout << "offline query size: " 
      << double(serialize_offline_query(offline_qry).size())/double(1000) << " KB" << endl;
    assert(equal_offline_query(offline_qry1, offline_qry));
    cout << "client generate offline query done." << endl;

    // server generates offline reply
    auto server_offline_st = chrono::high_resolution_clock::now();
    OfflineReply offline_reply = server.generate_offline_reply_fast(offline_qry1, 1);
    OfflineReply offline_reply1 = deserialize_offline_reply(serialize_offline_reply(offline_reply));
    cout << "offline reply size: " 
      << double(serialize_offline_reply(offline_reply).size())/double(1000) << " KB" << endl;

    assert(equal_offline_reply(offline_reply1, offline_reply1));

    auto server_offline_ed = chrono::high_resolution_clock::now();
    auto server_offline_time = std::chrono::duration_cast<std::chrono::microseconds>
            (server_offline_ed - server_offline_st).count();
    cout << "server generate offline reply done in " << server_offline_time << " microsec." << endl;

    // client stores hints locally
    client.update_local_hints(offline_reply1);

    // client generates online query
    OnlineQuery online_qry;
    uint32_t qry_idx = rand() % db_size;
    online_qry = client.generate_online_query(qry_idx);
    OnlineQuery online_qry1 = deserialize_online_query(serialize_online_query(online_qry));
    cout << "online query size: " 
      << double(serialize_online_query(online_qry).size()) / double(1000) << " KB" << endl;
    assert(equal_online_query(online_qry1, online_qry));
    cout << "client generate online query done."<< endl;

    // server generates online reply
    auto server_online_st = chrono::high_resolution_clock::now();
    OnlineReply online_reply = server.generate_online_reply(online_qry1, 1);
    OnlineReply online_reply1 = deserialize_online_reply(serialize_online_reply(online_reply));
    
    cout << "online reply size: " 
      << double(serialize_online_reply(online_reply).size()) / double(1000) << " KB" << endl;

    auto server_online_ed = chrono::high_resolution_clock::now();
    auto server_online_time = std::chrono::duration_cast<std::chrono::microseconds>
            (server_online_ed - server_online_st).count();
    cout << "server generate online reply done in " << server_online_time << " microsec." << endl;

    // client reconstructs block
    Block blk = client.recover_block(online_reply1);
    cout << "client recover done."  << endl;

    // check correctness
    if (blk == my_database[qry_idx]) {
        cout << "success" << endl;
    } else cout << "fail" << endl;

    // client refreshes
    OnlineQuery refresh_query = client.generate_refresh_query(qry_idx); 

    cout << "refresh query size: " 
      << double(serialize_online_query(refresh_query).size()) / double(1000) << " KB" << endl;

    auto server_refresh_st = chrono::high_resolution_clock::now();
    OnlineReply refresh_reply = server.generate_online_reply(refresh_query, 1);
    auto server_refresh_ed = chrono::high_resolution_clock::now();
    auto server_refresh_time = std::chrono::duration_cast<std::chrono::microseconds>
            (server_refresh_ed - server_refresh_st).count();
    cout << "refresh reply size: " 
      << double(serialize_online_reply(refresh_reply).size()) / double(1000) << " KB" << endl;
    cout << "server generate refresh reply done in " << server_refresh_time << " microsec." << endl;

    client.sets[client.cur_qry_setno].hint = blk ^ refresh_reply.parity;   // update hints

    return 0;
}

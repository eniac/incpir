#include <iostream>
#include <fstream>
#include <random>
#include <openssl/rand.h>
#include <unistd.h>
#include <chrono>

#include "../src/adprp.hpp"
#include "../src/client.hpp"
#include "../src/server.hpp"

using namespace std;

// data from Tor trace, one batch of additions in three days 
#define NUMDAYS 29
const int torarr[NUMDAYS] = {471, 618, 806, 450, 293, 492, 321, 810, 343,
                        100, 534, 280, 480, 108, 280, 125, 305, 96, 434,
                        192, 276, 42, 154, 37, 180, 87, 155, 92, 63};


int main(int argc, char* argv[])
{
    int opt;
    int cfreq = 100;    // Number of queries between database updates
    int cflag = 0;
    while ((opt = getopt(argc, argv, "q:c")) != -1) {
        if (opt == 'q')
            cfreq = atoi(optarg);
        else if (opt == 'c')
            cflag = 1;
    }

    // Set up global parameters
    SetupParams my_params;
    my_params.dbrange = 7000;  // Database size
    my_params.setsize = 83;    // Set size is sqrt(dbrange)
    my_params.replica = 13;    // Can adjust this for query failure rate
    my_params.nbrsets = (my_params.dbrange / my_params.setsize) * my_params.replica;

    // Create test database
    std::random_device rd;
    Database my_database(my_params.dbrange);
    for (int i = 0; i < my_params.dbrange; i++) {
        auto val = rd() % 1000000;
        my_database[i] = val;   // create enough non-zeros
        my_database[i] |=  (my_database[i] << 100);
        my_database[i] |=  (my_database[i] << 100);
    }

    // Initialize server
    PIRServer server(my_params.dbrange, my_params.setsize, my_params.nbrsets);
    server.set_database(&my_database);

    // Initialize client
    PIRClient client(my_params.dbrange, my_params.setsize, my_params.nbrsets);

    // Prep: client issues offline query
    auto c_prep_st = chrono::high_resolution_clock::now();
    OfflineQuery offline_query = client.generate_offline_query();
    auto c_prep_ed = chrono::high_resolution_clock::now();
    auto c_prep_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_prep_ed - c_prep_st).count();
    cout << "Prep: client generates offline query in "
    << double(c_prep_time)/double(ONEMS) << " ms" << endl;

    // Prep: server generates offline reply
    auto s_prep_st = chrono::high_resolution_clock::now();
    OfflineReply offline_reply = server.generate_offline_reply(offline_query, 1);
    auto s_prep_ed = chrono::high_resolution_clock::now();
    auto s_prep_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_prep_ed - s_prep_st).count();
    cout << "Prep: server generates offline reply in "
    << double(s_prep_time)/double(ONESEC) << " sec" << endl;
    
    // Client stores hints locally
    client.update_parity(offline_reply);

    // Client picks a random query index
    client.cur_qry_idx = rand() % my_params.dbrange;

    // Client issues an online query
    auto c_query_st = chrono::high_resolution_clock::now();
    OnlineQuery online_query = client.generate_online_query(client.cur_qry_idx);
    auto c_query_ed = chrono::high_resolution_clock::now();
    auto c_query_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_query_ed - c_query_st).count();
    cout << "Query: client generates online query in "
    << double(c_query_time)/double(ONEMS) << " ms" << endl;

    // Server generates online reply
    auto s_query_st = chrono::high_resolution_clock::now();
    OnlineReply online_reply = server.generate_online_reply(online_query, 1);
    auto s_query_ed = chrono::high_resolution_clock::now();
    auto s_query_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_query_ed - s_query_st).count();
    cout << "Reply: server generates online reply in "
    << double(s_query_time)/double(ONEMS) << " ms" << endl;

    // Client reconstructs queried block
    Block blk = client.query_recov(online_reply);

    // Check correctness
    if (blk == my_database[client.cur_qry_idx])  {
        cout << "success" << endl;
    } else cout << "fail" << endl;

    // Client generates refresh query
    auto c_refresh_st = chrono::high_resolution_clock::now();
    OnlineQuery refresh_query = client.generate_refresh_query(client.cur_qry_idx);
    auto c_refresh_ed = chrono::high_resolution_clock::now();
    auto c_refresh_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_refresh_ed - c_refresh_st).count();
    cout << "Refresh: client generates refresh query in "
    << double(c_refresh_time)/double(ONEMS) << " ms" << endl;

    auto s_refresh_st = chrono::high_resolution_clock::now();
    OnlineReply refresh_reply = server.generate_online_reply(refresh_query, 1);
    auto s_refresh_ed = chrono::high_resolution_clock::now();
    auto s_refresh_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_refresh_ed - s_refresh_st).count();
    cout << "Refresh: server generates refresh reply in "
    << double(s_refresh_time)/double(ONEMS) << " ms" << endl;

    // Client updates hints for a new set after refresh
    client.refresh_recov(refresh_reply);

    // Query again, test correctness
    client.cur_qry_idx = rand() % my_params.dbrange;
    online_query = client.generate_online_query(client.cur_qry_idx);
    online_reply = server.generate_online_reply(online_query, 1);
    blk = client.query_recov(online_reply);
    if (blk == my_database[client.cur_qry_idx]) {
        std::cout << "success" << std::endl;
    } else {
        std::cout << "fail" << std::endl;
    }

    cout << " ==== Simulate with Tor data trace ===="  << endl;
    vector<int> update_time;
    vector<double> storage_inc;
    int fail_cnt = 0;

    for (int B = 0; B < NUMDAYS-1; B++) {
        int nbr_add = torarr[B];
        if (nbr_add == 0) continue;

        std::vector<Block> torvec(nbr_add);
        for (int i = 0; i < nbr_add; i++) {
            torvec[i] = rand() % 100000;
        }
        
        // Issue cfreq queries between database updates 
        for (int q = 0; q < cfreq; q++) {
            if (B == 0) continue;
            
            client.cur_qry_idx = rand() % my_params.dbrange;
            online_query = client.generate_online_query(client.cur_qry_idx);

            auto svr_normal_st = std::chrono::high_resolution_clock::now();
            online_reply = server.generate_online_reply(online_query, 1);
            auto svr_normal_ed = std::chrono::high_resolution_clock::now();
            auto svr_normal_time = std::chrono::duration_cast<std::chrono::microseconds>
                    (svr_normal_ed - svr_normal_st).count();

            blk = client.query_recov(online_reply);
            if (blk != my_database[client.cur_qry_idx])
                fail_cnt++;
            
            refresh_query = client.generate_refresh_query(client.cur_qry_idx);
            refresh_reply = server.generate_online_reply(refresh_query, 1);
            client.refresh_recov(refresh_reply);
        }

        //  Print client storage 
        unsigned long long storage = 0;
        for (int i = 0; i < my_params.nbrsets; i++) {
            storage += ((client.localhints.sets[i].aux.size()-1) * 32 * 2);
        }
        storage_inc.push_back(double(storage) / double(ONEKB));

        // Update database  
        server.add_elements(nbr_add, torvec);
        UpdateQueryAdd offline_add_qry = client.batched_addition_query(nbr_add);

        auto scomp_st = std::chrono::high_resolution_clock::now();
        OfflineReply offline_add_reply = server.batched_addition_reply(offline_add_qry);
        auto scomp_ed = std::chrono::high_resolution_clock::now();
        auto scomp_time = std::chrono::duration_cast<std::chrono::microseconds>(scomp_ed - scomp_st).count();
        update_time.push_back(scomp_time);

        client.update_parity(offline_add_reply);
    }
    std::cout << "Query failure frequency = " << double(fail_cnt)/(cfreq*(NUMDAYS-1)) << std::endl;

    ofstream outfile;
    outfile.open("server_comp.dat"); 
    outfile << s_prep_time;
    for (int i = 0; i < update_time.size(); i++) {
        outfile << ", " << update_time[i];
    }
    outfile << "\n";
    outfile.close();

    if (cflag) {
        double init_storage = double(client.nbrsets * (224 + 16000)) / double(ONEKB);

        outfile.open("client_storage.dat", std::ios_base::app);
        outfile << 100*(storage_inc[0]/init_storage);	// percentage
        for (int i = 1; i < storage_inc.size(); i++) {
            outfile << ", " << 100*(storage_inc[i]/init_storage);
        }
        outfile << "\n";
        outfile.close();
    }
    
    return 0;
}

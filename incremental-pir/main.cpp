#include <iostream>
#include <random>
#include <openssl/rand.h>
#include <unistd.h>
#include <iomanip>

#include "src/adprp.hpp"
#include "src/client.hpp"
#include "src/server.hpp"


#include <chrono>

using namespace std;

typedef struct {
    double client_prep;       // ms
    double client_query;      // ms
    double client_refresh;    // ms
    double client_incprep;    // sec
    double server_prep;       // sec
    double server_resp;       // ms
    double server_incprep;    // sec
    double comm_prep;         // mb
    double comm_query;        // kb
    double comm_refresh;      // kb
    double comm_incprep;      // mb
} BenchDat;

void print_benchdat(BenchDat benchdat) {
  cout << fixed << setprecision(2);
  cout << "Client" << endl;
  cout << "\t Prep " << benchdat.client_prep << " ms" << endl;
  cout << "\t Query " << benchdat.client_query << " ms" << endl;
  cout << "\t Refresh " << benchdat.client_refresh << " ms" << endl;
  cout << "\t IncPrep " << benchdat.client_incprep << " sec" << endl;
  
  cout << "Server" << endl;
  cout << "\t Prep " << benchdat.server_prep << " sec" << endl;
  cout << "\t Resp " << benchdat.server_resp << " ms" << endl;
  cout << "\t IncPrep " << benchdat.server_incprep << " sec" << endl;

  cout << "Comm" << endl;
  cout << "\t Prep " << benchdat.comm_prep << " MB" << endl;
  cout << "\t Query " << benchdat.comm_query << " KB" << endl;
  cout << "\t Refresh " << benchdat.comm_refresh << " KB" << endl;
  cout << "\t IncPrep " << benchdat.comm_incprep << " MB" << endl;
}

int main(int argc, char* argv[])
{
    // set up global parameters
    SetupParams my_params;
    int opt;
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        if (opt == 'd') {
            my_params.dbrange = (1<< atoi(optarg));
            my_params.setsize = (1 << (atoi(optarg)/2));
        }
    }
    my_params.replica = 12;
    my_params.nbrsets = (my_params.dbrange / my_params.setsize) * my_params.replica;

    BenchDat benchdat;

    // create test database
    std::random_device rd;
    Database my_database(my_params.dbrange);
    for (int i = 0; i < my_params.dbrange; i++) {
        auto val = rd() % 1000000;
    }

    // initialize server
    PIRServer server(my_params.dbrange, my_params.setsize, my_params.nbrsets);
    server.set_database(&my_database);

    // initialize client
    PIRClient client(my_params.dbrange, my_params.setsize, my_params.nbrsets);

    // Prep: client issues offline query
    auto c_prep_st = chrono::high_resolution_clock::now();
    OfflineQuery offline_query = client.generate_offline_query();
    auto c_prep_ed = chrono::high_resolution_clock::now();
    auto c_prep_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_prep_ed - c_prep_st).count();
    benchdat.client_prep = double(c_prep_time)/double(ONEMS);

    // Prep: server generates offline reply
    auto s_prep_st = chrono::high_resolution_clock::now();
    OfflineReply offline_reply = server.generate_offline_reply(offline_query, 1);
    auto s_prep_ed = chrono::high_resolution_clock::now();
    auto s_prep_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_prep_ed - s_prep_st).count();
    benchdat.server_prep = double(s_prep_time)/double(ONESEC);
    benchdat.comm_prep = double(my_params.nbrsets * (224+BlockLen)) / double(8000000);

    // client stores hints locally
    client.update_parity(offline_reply);

    // client picks a random query index
    client.cur_qry_idx = rand() % my_params.dbrange;

    // client issues an online query
    auto c_query_st = chrono::high_resolution_clock::now();
    OnlineQuery online_query = client.generate_online_query(client.cur_qry_idx);
    auto c_query_ed = chrono::high_resolution_clock::now();
    auto c_query_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_query_ed - c_query_st).count();
    benchdat.client_query = double(c_query_time)/double(ONEMS);


    benchdat.comm_query = double(online_query.indices.size() * 64) / double(8000);
    benchdat.comm_refresh = benchdat.comm_query;

    // server generates online reply
    auto s_query_st = chrono::high_resolution_clock::now();
    OnlineReply online_reply = server.generate_online_reply(online_query, 1);
    auto s_query_ed = chrono::high_resolution_clock::now();
    auto s_query_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_query_ed - s_query_st).count();
    benchdat.server_resp = double(s_query_time)/double(ONEMS);

    // client reconstructs queried block
    Block blk = client.query_recov(online_reply);

    // client generates refresh query
    auto c_refresh_st = chrono::high_resolution_clock::now();
    OnlineQuery refresh_query = client.generate_refresh_query(client.cur_qry_idx);
    auto c_refresh_ed = chrono::high_resolution_clock::now();
    auto c_refresh_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_refresh_ed - c_refresh_st).count();
    benchdat.client_refresh = double(c_refresh_time)/double(ONEMS);
  
    auto s_refresh_st = chrono::high_resolution_clock::now();
    OnlineReply refresh_reply = server.generate_online_reply(refresh_query, 1);
    auto s_refresh_ed = chrono::high_resolution_clock::now();
    auto s_refresh_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_refresh_ed - s_refresh_st).count();

    // client updates hints for a new set after refresh
    client.refresh_recov(refresh_reply);

    // query again, test correctness
    client.cur_qry_idx = rand() % my_params.dbrange;

    online_query = client.generate_online_query(client.cur_qry_idx);
    online_reply = server.generate_online_reply(online_query, 1);

    blk = client.query_recov(online_reply);


    // test IncPrep

    // setup additions
    double perct_add = 0.01;
    int nbr_add = perct_add * my_params.dbrange;
    my_params.dbrange += nbr_add;

    // assign random value to added items
    std::vector<Block> v(nbr_add);
    for (int i = 0; i < nbr_add; i++) {
        v[i] = rand()%100000;
        // create enough non-zeros
        v[i] |= (v[i] << 100);
    }

    // DBUpd: server adds new items
    server.add_elements(nbr_add, v);

    // IncPrep: client issues hint update request
    auto c_add_st = chrono::high_resolution_clock::now();
    UpdateQueryAdd offline_add_qry = client.batched_addition_query(nbr_add);
    auto c_add_ed = chrono::high_resolution_clock::now();
    auto c_add_time = std::chrono::duration_cast<std::chrono::microseconds>
            (c_add_ed - c_add_st).count();
    benchdat.client_incprep = double(c_add_time)/double(ONESEC);

    // IncPrep: server generates hint update response
    auto s_add_st = chrono::high_resolution_clock::now();
    OfflineReply offline_add_reply = server.batched_addition_reply(offline_add_qry);
    auto s_add_ed = chrono::high_resolution_clock::now();
    auto s_add_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_add_ed - s_add_st).count();
    benchdat.server_incprep = double(s_add_time)/double(ONESEC);
    benchdat.comm_incprep = double(my_params.nbrsets * (KeyLen + 32 + 64 + 128 + BlockLen)) / double(8000000);


    // IncPrep: client updates local hints
    client.update_parity(offline_add_reply);

    // test client online query after one batched addition
    client.cur_qry_idx = my_database.size() - 2;
    online_query = client.generate_online_query(client.cur_qry_idx);

    // server generates online reply
    online_reply = server.generate_online_reply(online_query, 1);

    // client reconstruct the queried block
    blk = client.query_recov(online_reply);

    // check correctness
    // fail with small probability
    if (blk == my_database[client.cur_qry_idx])
        std::cout << "success!" << std::endl;
    else std::cout << "fail!" << std::endl;

    print_benchdat(benchdat);
    
    return 0;
}


#include "net_client.cpp"
#include <thread>

vector<double> handle_time;

void online_query(NetClient& client, Database& my_database, int db_size, int id, vector<Block>&query_res, OnlineQuery& online_qry, string ip) {
    double total_time = 0.0;
    Block blk = client.online_query(ip, online_qry, total_time);
    cout << "total time: " << total_time << " for id: " << id << endl;
    query_res[id] = blk;
    handle_time[id] = total_time;
}

int main(int argc, char *argv[]) {
    uint32_t db_size = 7000;
    uint32_t set_size = 80;
    uint32_t nbr_sets = 90*12;

    int offer_load = 10;
    int opt;
    int time_period = 1;
    string ip = "0.0.0.0:6666";
    while ((opt = getopt(argc, argv, "l:i:t:d:s:n:")) != -1) {
        if (opt == 'l') {
            offer_load = atoi(optarg);
        }
        else if (opt == 'i') {
            ip = string(optarg);
        }
        else if (opt == 't') {
            time_period = atoi(optarg);
        }
        else if (opt == 'd') {
            db_size = atoi(optarg);
        }
        else if (opt == 's') {
            set_size = atoi(optarg);
        }
        else if (opt == 'n') {
            nbr_sets = atoi(optarg);
        }
    }

    // init handle_time
    handle_time = vector<double>(offer_load, 0.0);
    assert(handle_time.size() == offer_load);

    unsigned int sleep_period = 1000000/100 * time_period / offer_load;
    cout << "sleep period: " << sleep_period << endl;
    srand(100);
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

    vector<Block> query_res(offer_load);
    vector<int> query_idx(offer_load);
    vector<OnlineQuery> querys(offer_load);
    vector<NetClient> clients;
    vector<thread> t_vec;
    NetClient client(db_size, set_size, nbr_sets);
    client.offline_query(ip);
    for (int i = 0; i < offer_load; i++) {
        int qry_idx = rand() % db_size;
        OnlineQuery online_qry = client.generate_online_query(qry_idx);
        query_idx[i] = qry_idx;
        querys[i] = online_qry;
    }
    system_clock::time_point start, end;
    start = system_clock::now();
    for (int i = 0; i < offer_load; i++) {
        thread t(&online_query, ref(client), ref(my_database), db_size, i, ref(query_res), ref(querys[i]), ip);
        t_vec.push_back(move(t));
        usleep(sleep_period);
    }

    for (auto& th : t_vec) {
        th.join();
    }
    end = system_clock::now();
    cout << "time spent: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;

    double total_handle_time = 0.0;
    for (int i = 0; i < handle_time.size(); i++) {
        assert(handle_time[i] != 0.0);
        total_handle_time += handle_time[i];
    }

    cout << "avg handle time: " << total_handle_time / offer_load << endl;

    return 0;
}

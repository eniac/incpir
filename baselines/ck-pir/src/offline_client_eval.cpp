#include "net_client.cpp"
#include <thread>

vector<double> handle_time;

void offline_baseline_query(NetClient& client, int id, OfflineQuery& offline_qry, string ip) {
    double total_time = 0.0;
    client.offline_query_baseline(ip, offline_qry, total_time);
    cout << "total time baseline: " << total_time << " for id: " << id << endl;
    handle_time[id] = total_time;
}

int main(int argc, char *argv[]) {
    uint32_t db_size = 70000;
    uint32_t lgn = 17; // database n = 2^lgn

    int offer_load = 10;
    int opt;
    int time_period = 1;
    string ip = "0.0.0.0:6666";
    while ((opt = getopt(argc, argv, "l:i:t:d:g:")) != -1) {
        if (opt == 'l') {
            offer_load = atoi(optarg);
        }
        else if (opt == 'i') {
            ip = string(optarg);
        }
        else if (opt == 't') {
            time_period = atoi(optarg);
        }
        else if (opt == 'g') {
            lgn = atoi(optarg);
        }
        else if (opt == 'd') {
            db_size = atoi(optarg);
        }
    }

    // init handle_time
    handle_time = vector<double>(offer_load, 0.0);
    assert(handle_time.size() == offer_load);

    unsigned int sleep_period = 1000000/100 * time_period / offer_load;
    cout << "sleep period: " << sleep_period << endl; 

    vector<OfflineQuery> querys;
    vector<NetClient> clients;
    vector<thread> t_vec;
    for (int i = 0; i < offer_load; i++) {
        int qry_idx = rand() % db_size;
        NetClient client(db_size, (1<<(lgn/2)), (1<<(lgn/2))*12);
        OfflineQuery offline_qry = client.generate_offline_query();
        querys.push_back(offline_qry);
        clients.push_back(client);
    }
    system_clock::time_point start, end;
    start = system_clock::now();
    for (int i = 0; i < offer_load; i++) {
        thread t(&offline_baseline_query, ref(clients[i]), i, ref(querys[i]), ip);
        t_vec.push_back(move(t));
        usleep(sleep_period);
    }

    for (auto& th : t_vec) {
        th.join();
    }
    end = system_clock::now();
    cout << "time spent: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;

    // for (int i = 0; i < offer_load; i++) {
    //     if (query_res[i] == my_database[query_idx[i]]) {
    //         std::cout << "recover success! ID: " << query_idx[i] << std::endl;
    //     } else {
    //         std::cout << "recover fail! ID: " << query_idx[i] << std::endl;
    //     }
    // }

    double total_handle_time = 0.0;
    for (int i = 0; i < handle_time.size(); i++) {
        assert(handle_time[i] != 0.0);
        total_handle_time += handle_time[i];
    }

    cout << "avg handle time: " << total_handle_time / offer_load << endl;
    return 0;
}


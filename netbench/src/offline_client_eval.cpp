#include "net_client.cpp"
#include <thread>

vector<double> handle_time;

void add_query(NetClient& client, Database& my_database, int db_size, int id, UpdateQueryAdd& add_qry, string ip) {
    double total_time = 0.0;
    client.add_query(ip, add_qry, total_time);
    cout << "total time: " << total_time << " for id: " << id << endl;
    handle_time[id] = total_time;
}

void offline_baseline_query(NetClient& client, Database& my_database, int db_size, int id, OfflineQuery& offline_qry, string ip) {
    double total_time = 0.0;
    client.offline_query_baseline(ip, offline_qry, total_time);
    cout << "total time baseline: " << total_time << " for id: " << id << endl;
    handle_time[id] = total_time;
}

int main(int argc, char *argv[]) {
    uint32_t db_size = 7000;
    uint32_t set_size = 80;
    uint32_t nbr_sets = 90*12;

    int offer_load = 10;
    int opt;
    string ip = "0.0.0.0:6666";
    int nbr_add = 70;
    bool baseline = false;
    int time_period = 1;
    while ((opt = getopt(argc, argv, "l:i:a:bd:s:n:t:")) != -1) {
        if (opt == 'l') {
            offer_load = atoi(optarg);
        }
        else if (opt == 'i') {
            ip = string(optarg);
        }
        else if (opt == 'a') {
            nbr_add = atoi(optarg);
        }
        else if (opt == 'b') {
            baseline = true;
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
        else if (opt == 't') {
            time_period = atoi(optarg);
        }
    }

    // init handle_time
    handle_time = vector<double>(offer_load, 0.0);
    assert(handle_time.size() == offer_load);

    cout << "set size: " << set_size << endl;
    cout << "nbr_sets: " << nbr_sets << endl;
    unsigned int sleep_period = 1000000/100 * time_period / offer_load;
    cout << "sleep period: " << sleep_period << endl;
    cout << "nbr_add: " << nbr_add << endl;
    cout << "db_size: " << db_size << endl;
    srand(100);
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

    vector<Block> query_res(offer_load);
    vector<int> query_idx(offer_load);
    vector<UpdateQueryAdd> querys(offer_load);
    vector<NetClient> clients;
    vector<thread> t_vec;
    vector<OfflineQuery> offline_querys;
    system_clock::time_point start, end;

    if (baseline) {
        for (int i = 0; i < offer_load; i++) {
            NetClient client(db_size, set_size, nbr_sets);
            offline_querys.push_back(client.generate_offline_query());
            clients.push_back(client);
        }
        start = system_clock::now();
        for (int i = 0; i < offer_load; i++) {
            thread t(&offline_baseline_query, ref(clients[i]), ref(my_database), db_size, i, ref(offline_querys[i]), ip);
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

    for (int i = 0; i < offer_load; i++) {
        NetClient client(db_size, set_size, nbr_sets);
        client.offline_query(ip);
        UpdateQueryAdd add_qry = client.batched_addition_query(nbr_add); 
        querys[i] = add_qry;
        clients.push_back(client);
    }
    start = system_clock::now();
    for (int i = 0; i < offer_load; i++) {
        thread t(&add_query, ref(clients[i]), ref(my_database), db_size, i, ref(querys[i]), ip);
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
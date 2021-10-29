#include "serial.hpp"
#include "net.hpp"
#include <thread>

class NetClient : public PIRClient {
public:
    NetClient(uint32_t lgn_) {
        lgn = lgn_;
        cout << "client created." << endl;
    }

    int get_fd(string ip) {
        int fd = connect_to_addr(ip);
        assert(fd > 0);
        return fd;
    }

    void online_query(string ip, ClientQuery online_qry, double& total_time, ServerReply& l, ServerReply& r) {
        system_clock::time_point start, end;
        start = system_clock::now();
        int fd = get_fd(ip);
        string q_str = serialize_query(online_qry);
        sendMsg(fd, q_str);
        string response;
        assert(recvMsg(fd, response));
        deserialize_reply(response, l, r);
        end = system_clock::now();
        total_time = duration_cast<std::chrono::duration<double>>(end - start).count();
        close(fd);
    }
};

vector<double> handle_time;

void query(NetClient& client, ClientQuery& qry, string ip, int id, int qry_idx) {
    double total_time = 0.0;
    ServerReply l, r;
    client.online_query(ip, qry, total_time, l, r);
    cout << "total time: " << total_time << " for id: " << id << endl;
    handle_time[id] = total_time;
    // hashdatastore::hash_type answer = _mm256_xor_si256(l.reply, r.reply);
    // if (_mm256_extract_epi64(answer, 0) == qry_idx) {
    //     cout << "PIR answer correct\n";
    // } else {
    //     cout << "PIR answer wrong\n";
    // }
}

int main(int argc, char* argv[]) {
    string ip = "0.0.0.0:6666";
    int opt;
    int offer_load = 1;
    int time_period = 1;
    int lgn = 16;
    while ((opt = getopt(argc, argv, "i:l:t:g:")) != -1) {
        if (opt == 'i') {
            ip = string(optarg);
        }
        else if (opt == 'l') {
            offer_load = atoi(optarg);
        }
        else if (opt == 't') {
            time_period = atoi(optarg);
        }
        else if (opt == 'g') {
            lgn = atoi(optarg);
        }
    }

    // init handle_time
    handle_time = vector<double>(offer_load, 0.0);
    assert(handle_time.size() == offer_load);

    cout << "offer load: " << offer_load << endl;
    vector<int> query_idx;
    vector<NetClient> clients;
    vector<thread> t_vec;
    vector<ClientQuery> querys;
    system_clock::time_point start, end;

    unsigned int sleep_period = 1000000/100 * time_period / offer_load;
    cout << "sleep period: " << sleep_period << endl;
    
    for (int i = 0; i < offer_load; i++) {
        NetClient client(lgn);
        int qry_idx = rand() % 8000;
        query_idx.push_back(qry_idx);
        querys.push_back(client.generate_query(qry_idx));
        clients.push_back(client);
    }

    start = system_clock::now();
    for (int i = 0; i < offer_load; i++) {
        thread t(&query, ref(clients[i]), ref(querys[i]), ip, i, query_idx[i]);
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
    // for (int i = 0; i < offer_load; i++) {
    //     int qry_idx = query_idx[i];
    //     ServerReply l = left_replys[i];
    //     ServerReply r = right_replys[i];
    //     hashdatastore::hash_type answer = _mm256_xor_si256(l.reply, r.reply);
    //     if (_mm256_extract_epi64(answer, 0) == qry_idx) {
    //         cout << "PIR answer correct\n";
    //     } else {
    //         cout << "PIR answer wrong\n";
    //     }
    // }
    return 0;
}


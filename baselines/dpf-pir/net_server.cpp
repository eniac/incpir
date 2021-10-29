#include "serial.hpp"
#include "net.hpp"

class NetServer : public PIRServer {

public:

    string online_handler(string recv_msg) {
        ClientQuery query = deserialize_query(recv_msg);
        ServerReply left_reply = generate_left_reply(query.left_query);
        ServerReply right_reply = generate_right_reply(query.right_query);
        string msg = serialize_reply(left_reply, right_reply);
        return msg;
    }

    NetServer(uint32_t lgn_, hashdatastore *database_, string ip, int offer_load, double time_period) {
        if (database_ == nullptr) {
            throw std::invalid_argument("database cannot be null");
        }

        lgn = lgn_;
        database = database_;
        cout << "server created." << endl;
        int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
        int sock_opt;
        assert(setsockopt(listen_fd, SOL_SOCKET, TCP_NODELAY | SO_REUSEADDR |
        SO_REUSEPORT, &sock_opt, sizeof(sock_opt)) >= 0);
        struct sockaddr_in servaddr = string_to_struct_addr(ip);
        assert(bind(listen_fd, (sockaddr *)&servaddr, sizeof(servaddr)) >= 0);
        assert(listen(listen_fd, 100) >= 0);

        system_clock::time_point total_start, total_end;
        bool started = false;
        int handled_task = 0;
        bool found = false;

        for (int i = 0; i < offer_load; i++) {
            // accept for one connection
            struct sockaddr_in clientaddr;
            socklen_t clientaddrlen = sizeof(clientaddr);
            int client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);
            cout << "connected\n";
            string recv_msg;
            if (!recvMsg(client_fd, recv_msg)) {
                cout << "receive msg fail\n";
                continue;
            }
            if (!started) {
                total_start = system_clock::now();
                started = true;
            }
            handled_task++;
            system_clock::time_point start, end;
            start = system_clock::now();
            string response = online_handler(recv_msg);
            end = system_clock::now();
            cout << "online handler: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
            assert(sendMsg(client_fd, response));
            close(client_fd);
            if (started) {
                total_end = system_clock::now();
                cout << "elapsed time: " << duration_cast<std::chrono::duration<double>>(total_end - total_start).count() << ", handled task: " << handled_task << endl;
                if (!found && duration_cast<std::chrono::duration<double>>(total_end - total_start).count() > time_period) {
                    cout << "FOUND throughput " << handled_task << " per " << time_period << "s" << endl;
                    found = true;
                }
            }
        }
        close(listen_fd);
        cout << "server finish\n";
    }
};

int main(int argc, char* argv[]) {
    int lgn = 16;
    int opt;
    int offer_load = 1;
    int time_period = 1;
    string ip = "0.0.0.0:6666";
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
    hashdatastore *dbptr;
    hashdatastore store;
    store.reserve(1ULL << lgn);
    for (size_t i = 0; i < (1ULL << lgn); i++) {
        store.push_back(_mm256_set_epi64x(i, i, i, i));
    }
    dbptr = &store;

    NetServer server(lgn, dbptr, ip, offer_load, 0.01*time_period);
    return 0;
}
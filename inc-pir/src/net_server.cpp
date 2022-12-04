#include "server.hpp"
#include "serial.hpp"
#include "net.hpp"

using namespace chrono;

class NetServer : public PIRServer {
    string offline_query_handler(string query_str) {
        system_clock::time_point start, end;
        start = system_clock::now();
        OfflineQuery offline_qry = deserialize_offline_query(query_str);
        end = system_clock::now();
        cout << "deserialize time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
        // Q: why 1 here?
        start = system_clock::now();
        OfflineReply offline_reply = generate_offline_reply(offline_qry, 1);
        end = system_clock::now();
        cout << "process time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
        start = system_clock::now();
        string res = serialize_offline_reply(offline_reply);
        end = system_clock::now();
        cout << "serialize time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
        return res;
    }

    string online_query_handler(string query_str) {
        OnlineQuery online_qry = deserialize_online_query(query_str);
        // Q: why 1 here?
        OnlineReply online_reply = generate_online_reply(online_qry, 1);
        return serialize_online_reply(online_reply);
    }
    
    string add_query_handler(string query_str) {
        system_clock::time_point start, end;
        start = system_clock::now();
        UpdateQueryAdd offline_add_qry = deserialize_offline_add_query(query_str);
        end = system_clock::now();
        cout << "deserialize time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
        start = system_clock::now();
        OfflineReply offline_add_reply = batched_addition_reply(offline_add_qry);
        end = system_clock::now();
        cout << "process time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
        start = system_clock::now();
        string res = serialize_offline_reply(offline_add_reply);
        end = system_clock::now();
        cout << "serialize time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
        return res;
    }

public:
    NetServer(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_): PIRServer(dbrange_, setsize_, nbrsets_) {}

    void run(Database* db_, string ip, double time_period) {
        set_database(db_);
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

        while (true) {
            // accept for one connection
            struct sockaddr_in clientaddr;
            socklen_t clientaddrlen = sizeof(clientaddr);
            int client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);
            // cout << "connected\n";
            string recv_msg;
            if (!recvMsg(client_fd, recv_msg)) {
                cout << "receive msg fail\n";
                continue;
            }
            interface::Query query;
            if (!query.ParseFromString(recv_msg)) {
                cout << "parse query failed\n";
                assert(0);
            }
            system_clock::time_point start, end;
            start = system_clock::now();
            string response;
            string query_str = query.msg();
            if (query.type() == interface::QueryType::OFFLINE) {
                response = offline_query_handler(query_str);
            }
            else if (query.type() == interface::QueryType::ONLINE) {
                if (!started) {
                    total_start = system_clock::now();
                    started = true;
                }
                handled_task++;
                response = online_query_handler(query_str);
            }

            else if (query.type() == interface::QueryType::ADD) {
                response = add_query_handler(query_str);
            }
            assert(sendMsg(client_fd, response));
            end = system_clock::now();
            cout << "handler time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
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

    }

    void run_offline(Database* db_, string ip, int nbr_add, int load, bool baseline, double time_period) {
        set_database(db_);
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

        for (int i = 0; i < load; i++) {
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
            interface::Query query;
            if (!query.ParseFromString(recv_msg)) {
                cout << "parse query failed\n";
                assert(0);
            }
            string response;
            string query_str = query.msg();
            if (query.type() == interface::QueryType::OFFLINE) {
                if (baseline && !started) {
                    total_start = system_clock::now();
                    started = true;
                }
                handled_task++;
                response = offline_query_handler(query_str);
            }
            else {
                cout << "should only receive offline request at the beginning\n";
                assert(0);
            }
            assert(sendMsg(client_fd, response));
            close(client_fd);
            if (baseline && started) {
                total_end = system_clock::now();
                cout << "elapsed time: " << duration_cast<std::chrono::duration<double>>(total_end - total_start).count() << ", handled task: " << handled_task << endl;
                if (!found && duration_cast<std::chrono::duration<double>>(total_end - total_start).count() > time_period) {
                    cout << "FOUND throughput " << handled_task << " per " << time_period << "s" << endl;
                    found = true;
                }
            }
        }

        if (baseline) {
            close(listen_fd);
            return;
        }

        // add elements
        std::vector<Block> v(nbr_add);
        for (int i = 0; i < nbr_add; i++) {
            v[i] = rand()%100000;
        }

        /* Server adds new data objects */
        add_elements(nbr_add, v);

        started = false;
        handled_task = 0;
        found = false;

        // start receiving batch add query
        for (int i = 0; i < load; i++) {
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
            interface::Query query;
            if (!query.ParseFromString(recv_msg)) {
                cout << "parse query failed\n";
                assert(0);
            }
            string response;
            string query_str = query.msg();
            if (query.type() == interface::QueryType::ADD) {
                 if (!started) {
                    total_start = system_clock::now();
                    started = true;
                }
                handled_task++;
                system_clock::time_point start, end;
                start = system_clock::now();
                response = add_query_handler(query_str);
                end = system_clock::now();
                cout << "add time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
            }
            else {
                cout << query.type() << endl;
                cout << "should only receive add request at the beginning\n";
                assert(0);
            }
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
        cout << "finish\n";
    }

    void run_refresh(Database* db_, string ip, double time_period) {
        set_database(db_);
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

        while (true) {
            // accept for one connection
            struct sockaddr_in clientaddr;
            socklen_t clientaddrlen = sizeof(clientaddr);
            int client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);
            // cout << "connected\n";
            string recv_msg;
            if (!recvMsg(client_fd, recv_msg)) {
                cout << "receive msg fail\n";
                continue;
            }
            interface::Query query;
            if (!query.ParseFromString(recv_msg)) {
                cout << "parse query failed\n";
                assert(0);
            }
            system_clock::time_point start, end;
            start = system_clock::now();
            string response;
            string query_str = query.msg();
            if (query.type() == interface::QueryType::OFFLINE) {
                response = offline_query_handler(query_str);
            }
            else if (query.type() == interface::QueryType::REFRESH) {
                if (!started) {
                    total_start = system_clock::now();
                    started = true;
                }
                handled_task++;
                response = online_query_handler(query_str);
            }

            else if (query.type() == interface::QueryType::ONLINE) {
                response = online_query_handler(query_str);
            }
            else {
                cout << "unsupported operation\n";
                assert(0);
            }
            assert(sendMsg(client_fd, response));
            end = system_clock::now();
            cout << "handler time: " << duration_cast<std::chrono::duration<double>>(end - start).count() << endl;
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

    }

};
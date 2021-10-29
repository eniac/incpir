#include "net_server.cpp"

int main(int argc, char* argv[]) {
    uint32_t db_size = 70000;
    uint32_t lgn = 17; // database n = 2^lgn
    string ip = "0.0.0.0:6666";
    int opt;
    int offer_load = 1;
    int time_period = 1;
    bool online = false;
    while ((opt = getopt(argc, argv, "i:l:t:od:g:")) != -1) {
        if (opt == 'i') {
            ip = string(optarg);
        }
        else if (opt == 'l') {
            offer_load = atoi(optarg);
        }
        else if (opt == 't') {
            time_period = atoi(optarg);
        }
        else if (opt == 'o') {
            online = true;
        }
        else if (opt == 'g') {
            lgn = atoi(optarg);
        }
        else if (opt == 'd') {
            db_size = atoi(optarg);
        }
    }
    std::random_device rd;
    Database my_database((db_size));
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }
    cout << "db_size: " << db_size << endl;
    cout << "online? " << online << endl;
    NetServer server;
    // time unit is 0.01 sec by default.
    server.run(db_size, &my_database, ip, 0.01*time_period, online);
    return 0;
}


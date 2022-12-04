#include "net_server.cpp"

int main(int argc, char *argv[]) {
    uint32_t db_size = 7000;
    uint32_t set_size = 80;
    uint32_t nbr_sets = 90*12;

    string ip = "0.0.0.0:6666";
    int opt;
    // by default time period is 0.01s
    int time_period = 1;
    while ((opt = getopt(argc, argv, "i:d:s:n:t:")) != -1) {
        if (opt == 'i') {
            ip = string(optarg);
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
    srand(100);
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }
    NetServer server(db_size, set_size, nbr_sets);
    server.run_refresh(&my_database, ip, 0.01*time_period);
    return 0;
}
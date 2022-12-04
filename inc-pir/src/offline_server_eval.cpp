#include "net_server.cpp"

int main(int argc, char *argv[]) {
    uint32_t db_size = 7000;
    uint32_t set_size = 80;
    uint32_t nbr_sets = 90*12;

    int offer_load = 10;
    int opt;
    int nbr_add = 70;
    string ip = "0.0.0.0:6666";
    bool baseline = false;
    // by default time period is 1s
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

    cout << "set size: " << set_size << endl;
    cout << "nbr_sets: " << nbr_sets << endl;
    cout << "db_size: " << db_size << endl;
    srand(100);
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }
    cout << "nbr_add: " << nbr_add << endl;
    NetServer server(db_size, set_size, nbr_sets);
    server.run_offline(&my_database, ip, nbr_add, offer_load, baseline, time_period*0.01);
    return 0;
}
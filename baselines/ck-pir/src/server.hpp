#ifndef _SERVER
#define _SERVER

#include "pir.hpp"

#include <openssl/rand.h>
#include <set>
#include <bitset>

class PIRServer {
public:
    uint32_t lgn;
    uint32_t db_size;
    Database *db = nullptr;

    PIRServer();

    void set_database(uint32_t db_size_, std::vector<Block> *db_);

    //OfflineReply generate_offline_reply(OfflineQuery offline_qry, uint32_t client_id);
    OfflineReply generate_offline_reply_fast(OfflineQuery offline_qry, uint32_t client_id);
    OnlineReply generate_online_reply(OnlineQuery online_qry, uint32_t client_id);

};

#endif

#ifndef _CLIENT
#define _CLIENT

#include "pir.hpp"

#include <stdint.h>
#include <set>

class PIRClient {
public:
    uint32_t lgn;
    uint32_t dbrange;
    uint32_t setsize;
    uint32_t nbrsets;

    uint32_t punc_x;

    int cur_qry_setno;

    std::vector<SetDesc> sets;

    PIRClient();

    void set_parms(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_);
    void generate_setkeys();

    OfflineQuery generate_offline_query();
    void update_local_hints(OfflineReply offline_reply);

    OnlineQuery generate_online_query(uint32_t desired_idx);

    Block recover_block(OnlineReply online_reply);


    // need to consider before and after add both
    OnlineQuery generate_refresh_query(uint32_t desired_idx);

};

#endif


#ifndef _CLIENT
#define _CLIENT


#include "pir.hpp"

#include <stdint.h>
#include <set>

typedef vector<tuple<uint32_t, uint32_t> > LINFO;

class PIRClient {
public:
    uint32_t dbrange;         // up-to-date database size
    uint32_t setsize;
    uint32_t nbrsets;
    LocalHints localhints;

    Key mk;                   // master key for kdf

    int cur_qry_idx;          // index for current query
    int cur_qry_setno;        // set number for current query
    Block cur_qry_blk;        // data block for current query

    PIRClient() {}
    // setup params and generate set keys
    PIRClient(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_);

    OfflineQuery generate_offline_query();

    OnlineQuery generate_online_query(uint32_t desired_idx);

    Block query_recov(OnlineReply online_reply);
    void refresh_recov(OnlineReply refresh_reply);

    // need to consider before and after add both
    OnlineQuery generate_refresh_query(uint32_t desired_idx);

    // updates: addition
    UpdateQueryAdd batched_addition_query(uint32_t nbr_add);

    void update_parity(OfflineReply offline_reply);


    //NewOnlineQuery query_easy(uint32_t desired_idx);
    //NewOnlineQuery query_hard(uint32_t desired_idx);

    //NewOnlineQuery checklist_generate_online_query(uint32_t desired_idx);

};

#endif


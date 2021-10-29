#include "server.hpp"
#include "adprp.hpp"
#include "pir.hpp"
#include <iostream>

using namespace std;

PIRServer::PIRServer(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_) {
    dbrange = dbrange_;
    setsize = setsize_;
    nbrsets = nbrsets_;

    cout << "server is created." << std::endl;
}

void PIRServer::set_database(Database *db_) {

    if (db_ == nullptr) {
        throw std::invalid_argument("database cannot be null");
    }

    db = db_;

    cout << "server database is set." << std::endl;
}

OfflineReply PIRServer::generate_offline_reply(OfflineQuery offline_qry, uint32_t client_id) {

    OfflineReply tmp;

    // init parity vector
    tmp.parities.resize(nbrsets);

    std::set<uint32_t> ts;

    // compute parity[i] for set i
    for (int s = 0; s < nbrsets; s++) {

        ts.clear();

        Key key = offline_qry.sets[s].sk;

        // eval PRF/PRP until reaches set size
        for (uint32_t j = 0; j < setsize; j++) {

            long long y = cycle_walk(j, dbrange, key);

            y = (y + offline_qry.sets[s].shift) % dbrange;

            tmp.parities[s] = tmp.parities[s] ^ ((*db)[y]);
        }

    }
    return tmp;
}

OnlineReply PIRServer::generate_online_reply(OnlineQuery online_qry, uint32_t client_id) {

    OnlineReply tmp;

    for (auto it = online_qry.indices.begin(); it != online_qry.indices.end(); it++) {

        tmp.parity = tmp.parity ^ ((*db)[(*it)]);
    }

    return tmp;
}

void PIRServer::add_elements(uint32_t nbr_add, std::vector<Block> v) {

    if (nbr_add != v.size()) {
        throw std::invalid_argument("server additions error");
    }

    for (int i = 0; i < nbr_add; i++) {
        (*db).push_back(v[i]);
    }

    dbrange += nbr_add;

    std::cout << "server adds " << nbr_add << " elements,"
    << " now database has " << (*db).size() << " elements in total" << std::endl;
}

//Block PIRServer::delete_element(uint32_t idx) {
//    Block blk = rand()%10000;
//    return blk ^ (*db)[idx];
//}
//
//Block PIRServer::edit_element(uint32_t idx, Block new_element) {
//    return new_element ^ (*db)[idx];
//}


OfflineReply PIRServer::batched_addition_reply(UpdateQueryAdd offline_add_qry) {

    OfflineReply tmp;

    // initialize OfflineReply
    tmp.parities.resize(nbrsets);

    for (int s = 0; s < nbrsets; s++) {

        // for each set

        // eval PRF to obtain the difference set

        Key key = offline_add_qry.diffsets[s].sk;

        std::vector<std::tuple<uint32_t, uint32_t> > prev = offline_add_qry.diffsets[s].aux_prev;
        std::vector<std::tuple<uint32_t, uint32_t> > cur = offline_add_qry.diffsets[s].aux_curr;

        // compute the difference of all slots except the last

        uint32_t kick_slot = prev.size(); // number of slot with indices kicked
        uint32_t accum_range = 0;

        for (int i = 0; i < kick_slot; i++) {

            // for each slot
            uint32_t range = std::get<0>(prev[i]);

            // derive key for each slot
            Key ki;

            if (i == 0) {
                ki = key;
            } else {
                ki = kdf(offline_add_qry.mk, key, i);
            }

            for (uint32_t ell = std::get<1>(cur[i]); ell < std::get<1>(prev[i]); ell++) {

                // DEBUG
                if (ell == range) {
                    throw std::invalid_argument("error");
                }

                uint32_t idx = cycle_walk(ell, range, ki);

                if (offline_add_qry.diffsets[s].shift != 0 && i == 0) {
                    idx = (idx + offline_add_qry.diffsets[s].shift ) % range;
                }

                tmp.parities[s] = tmp.parities[s] ^ (*db)[accum_range + idx];

            }

            accum_range += range;
        }

        // compute the last slot

        uint32_t new_slot = cur.size() - 1;
        uint32_t last_range = std::get<0>(cur[new_slot]);

        Key ki = kdf(offline_add_qry.mk, key, new_slot);
        //if (s==0) print_key(ki);
        // checked: ki is consistent with client online query

        for (uint32_t ell = 0; ell < std::get<1>(cur[new_slot]); ell++) {

            if (ell == last_range)
                throw invalid_argument("error in server eval");

            uint32_t idx = cycle_walk(ell, last_range, ki);

            tmp.parities[s] = tmp.parities[s] ^ (*db)[accum_range + idx];
        }


    }

    return tmp;
}



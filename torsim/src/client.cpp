#include "client.hpp"
#include "adprp.hpp"
#include "pir.hpp"
#include <math.h>
#include <cmath>
#include <random>
#include <openssl/rand.h>
#include <iostream>
using namespace std;


// given a set [s], probabilistically remove
// the desired index or another index
void probabilistic_remove(std::set<uint32_t> &s, const double pr, uint32_t idx) {

    std::random_device rd;
    std::mt19937 bgen(rd());
    std::bernoulli_distribution d(pr);

    if (d(bgen) == 0) {
        // remove desired index
        s.erase(idx);

    } else {

        s.erase(idx);

        // remove a random number in the set other than idx
        auto it = s.begin();

        // sample a random idx in the set
        int r = rand()%s.size();

        // increase iterator to idx
        for (int i = 0; i < r; i++, it++);

        // remove the element at idx
        s.erase(*it);

        s.insert(idx);
    }
}

// setup client
PIRClient::PIRClient(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_) {
    dbrange = dbrange_;
    setsize = setsize_;
    nbrsets = nbrsets_;

    // set master key
    uint8_t *mkptr = static_cast<uint8_t *>(malloc(KeyLen));
    RAND_bytes(mkptr, KeyLen);
    for (int i = 0; i < KeyLen; i++) {
        mk[i] = mkptr[i];
    }

    // setup succinct representation for each set
    for(int s = 0; s < nbrsets; s++) {

        SetDesc tmp;

        uint8_t *skptr = static_cast<uint8_t *>(malloc(KeyLen));
        RAND_bytes(skptr, KeyLen);
        for (int j = 0; j < KeyLen; j++) {
            tmp.sk[j] = skptr[j];
        }

        tmp.shift = rand() % dbrange;

        tmp.aux.emplace_back(std::make_tuple(dbrange, setsize));

        localhints.sets.push_back(tmp);
    }

    localhints.parities.resize(nbrsets);

    std::cout << "client is created." << std::endl;
}


// generate offline query
OfflineQuery PIRClient::generate_offline_query() {

    OfflineQuery tmp;

    for (int s = 0; s < nbrsets; s++) {

        SetDesc setdesc;
        setdesc.sk = localhints.sets[s].sk;
        setdesc.shift = localhints.sets[s].shift;
        setdesc.aux = localhints.sets[s].aux;

        tmp.sets.push_back(setdesc);
    }
    return tmp;
}


// update local hints
void PIRClient::update_parity(OfflineReply offline_reply) {

    for (int s = 0; s < nbrsets; s++) {
        localhints.parities[s] = localhints.parities[s] ^ offline_reply.parities[s];
    }
}

// generate online query:
OnlineQuery PIRClient::generate_online_query(uint32_t desired_idx) {

    // check index validity
    if (desired_idx >= dbrange)
        throw invalid_argument("query index invalid");

    OnlineQuery online_query;
    std::set<uint32_t> tmp;

    // find a set containing desired index
    int setno = -1;

    for (int s = 0; s < nbrsets; s++) {
        // expand a set: eval on each range

        uint32_t accum_range = 0;

        for (int slot = 0; slot < localhints.sets[s].aux.size(); slot++) {

            uint32_t range = std::get<0>(localhints.sets[s].aux[slot]);

            if (desired_idx >= accum_range && desired_idx < accum_range + range) {

                uint32_t inv;

                if (localhints.sets[s].shift != 0 && slot == 0) {
                    // actually happen only in the first slot
                    uint32_t shift_idx = (desired_idx + range - localhints.sets[s].shift) % range;

                    inv = inv_cycle_walk(shift_idx, range, localhints.sets[s].sk);

                } else {

                    // subsequent slot
                    Key ki = kdf(mk, localhints.sets[s].sk, slot);

                    inv = inv_cycle_walk(desired_idx - accum_range, range, ki);}

                if (inv < std::get<1>(localhints.sets[s].aux[slot])) {
                    setno = s;
                    goto Found;
                }
            }
            accum_range += range;
        }
    }

    Found:
    if (setno == -1) {
        // happen with low probability
        cout << "client cannot find its desired index. skip." << endl;
        setno = 0;
#ifdef DEBUG
        exit(0);
#endif
    }


    // expand this set to clear indices
    cur_qry_setno = setno;
    uint32_t accum_range = 0;

    for (int slot = 0; slot < localhints.sets[setno].aux.size(); slot++) {

        if (std::get<1>(localhints.sets[setno].aux[slot]) > dbrange) continue;

        uint32_t range = std::get<0>(localhints.sets[setno].aux[slot]);

        Key ki;

        if (slot == 0) {
            ki = localhints.sets[setno].sk;

        } else {
            ki = kdf(mk, localhints.sets[setno].sk, slot);
        }

        for (uint32_t x = 0; x < std::get<1>(localhints.sets[setno].aux[slot]); x++) {

            uint32_t y = cycle_walk(x, range, ki);

            if (localhints.sets[cur_qry_setno].shift != 0 && slot == 0)
                y = (y + localhints.sets[cur_qry_setno].shift) % range;

            tmp.insert(y + accum_range);
        }
        accum_range += range;
    }


    // probabilistically remove the desired index
    // or another index from the set
    double pr = double(setsize-1) / double(dbrange);
    probabilistic_remove(tmp, pr, desired_idx);

    for (auto it = tmp.begin(); it != tmp.end(); it++) {
        online_query.indices.push_back(*it);
    }
    return online_query;
}


// helper function: compute whether an element is in tuple list
int is_in_tuple (uint32_t r, std::vector< std::tuple<uint32_t, uint32_t> > v) {
    int flag = -1;
    for (int i = 0; i < v.size(); i++) {
        if (r == std::get<0>(v[i])) {
            flag = i;
            break;
        }
    }
    return flag;
}

// reconstruct a queried block
Block PIRClient::query_recov(OnlineReply online_reply) {
    cur_qry_blk = online_reply.parity ^ localhints.parities[cur_qry_setno];
    return cur_qry_blk;
}

// refresh (update hints)
void PIRClient::refresh_recov(OnlineReply refresh_reply) {
    localhints.parities[cur_qry_setno] = refresh_reply.parity ^ cur_qry_blk;
}


// client hint update request for IncPrep (one batch addition)
UpdateQueryAdd PIRClient::batched_addition_query(uint32_t nbr_add) {

    uint32_t prev_range = dbrange;
    dbrange = prev_range + nbr_add;

    UpdateQueryAdd tmp;
    tmp.mk = mk;
    tmp.setsize = setsize;
    tmp.nbrsets = nbrsets;


    // get ell for all sets
    vector<LINFO> sets_linfo;
    sets_linfo.resize(nbrsets);

    for (int i = 0; i < nbr_add; i++) {

        // sample db_size+i into every set
        uint32_t cur_idx = prev_range + i;
        double pr = double(setsize) / double(cur_idx);
        std::random_device rd;
        std::mt19937 bgen(rd());
        std::bernoulli_distribution d(pr);

        for (int s = 0; s < nbrsets; s++) {

            // choose a random number e.g. 3 in [setsize]
            uint32_t r = rand() % setsize;

            // push_back tuple e.g. (3, n+i) based on sampling
            if (d(bgen) == 1) {
                // if r == previous r in some tuple, replace n+prev with n+cur
                // otherwise push back
                int flag = is_in_tuple(r, sets_linfo[s]);
                if (flag != -1) {
                    std::get<1>(sets_linfo[s][flag]) = cur_idx;
                } else {
                    sets_linfo[s].emplace_back(std::make_tuple(r, cur_idx)); // update client set
                }
            }
        }
    }

    tmp.diffsets.resize(nbrsets);

    for (int s = 0; s < nbrsets; s++) {
        // keep prev sideinfo
        tmp.diffsets[s].aux_prev = localhints.sets[s].aux;

        // get number ell
        uint32_t ell = uint32_t(sets_linfo[s].size());

        // update side info of previous ranges
        double lsum = 0;

        std::vector<uint32_t> cntr;
        cntr.resize(localhints.sets[s].aux.size());

        for (int i = 0; i < localhints.sets[s].aux.size(); i++) {
            lsum += std::get<1>(localhints.sets[s].aux[i]);
            cntr[i] = uint32_t(lsum);
        }

        // kick out ell random indices
        for (int i = 0; i < ell; i++) {
            uint32_t fall = rand() % uint32_t(lsum);
            if (fall < cntr[0]) {
                std::get<1>(localhints.sets[s].aux[0])--;
            } else {
                for (int cnt = 0; cnt < cntr.size() - 1; cnt++) {
                    if (fall >= cntr[cnt] && fall < cntr[cnt+1]) {
                        std::get<1>(localhints.sets[s].aux[cnt+1])--;
                        break;
                    }
                }
            }
        }

        localhints.sets[s].aux.emplace_back(std::make_tuple(nbr_add, ell));

        tmp.diffsets[s].sk = localhints.sets[s].sk;
        tmp.diffsets[s].aux_curr = localhints.sets[s].aux;
        tmp.diffsets[s].shift = localhints.sets[s].shift;
    }
    return tmp;
}

// client generate refresh query
OnlineQuery PIRClient::generate_refresh_query(uint32_t desired_idx) {

    OnlineQuery refresh_query;
    std::set<uint32_t> tmp;

    // identify which set
    uint32_t setno = cur_qry_setno;

    // generate new key
    uint8_t *nsk = static_cast<uint8_t *>(malloc(KeyLen));
    RAND_bytes(nsk, KeyLen);
    for (int i = 0; i < KeyLen; i++) {
        localhints.sets[setno].sk[i] = nsk[i];
    }

    uint32_t r = rand() % setsize;
    uint32_t y = cycle_walk(r, dbrange, localhints.sets[setno].sk);

    // shift to get i
    localhints.sets[setno].shift = (desired_idx + dbrange - y) % dbrange; // shift is positive number

    // set new aux
    localhints.sets[setno].aux.clear();
    localhints.sets[setno].aux.emplace_back(std::make_tuple(dbrange, setsize));

    // eval prp
    for (uint32_t i = 0; i < setsize; i++) {
        uint32_t v = cycle_walk(i, dbrange, localhints.sets[setno].sk);
        v = (v + localhints.sets[setno].shift) % dbrange;
        tmp.insert(v);
    }

    // remove i with probabilistically
    double pr = double(setsize-1) / double(dbrange);
    probabilistic_remove(tmp, pr, desired_idx);

    for (auto it = tmp.begin(); it != tmp.end(); it++) {
        refresh_query.indices.push_back(*it);
    }
    return refresh_query;
}


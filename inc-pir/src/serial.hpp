#include "interface.pb.h"
#include <iostream>
#include <math.h>
#include <ctime>
#include <string>
#include <sstream>
#include <cstring>
#include <array>
#include <chrono>

#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

using namespace std;
using namespace chrono;

string serialize_online_reply(OnlineReply reply) {
    string res;
    interface::OnlineReply r;
    for (int j = 0; j < 16000/64; j++) {
        uint64_t result = 0;
        uint64_t mask = 1;
        for (int k = j*64; k < (j+1)*64; k++) {
            if (reply.parity[k] == 1)
                result |= mask;
            mask <<= 1;
        }
        r.add_reply(result);
    }
    r.SerializeToString(&res);
    return res;
}

OnlineReply deserialize_online_reply(string msg) {
    interface::OnlineReply r;
    if (!r.ParseFromString(msg)) {
        cout << "deserialize online reply failed\n";
        assert(0);
    }
    OnlineReply reply;
    bitset<16000> bitsets(0);
    for (int j = 0; j < 16000/64; j++) {
        uint64_t val = r.reply()[j];
        uint64_t mask = 1;
        for (int k = j*64; k < (j+1)*64; k++) {
            if ((val&mask) != 0)
                bitsets[k] = 1;
            else
                bitsets[k] = 0;
            mask <<= 1;
        }
    }
    reply.parity = bitsets;
    return reply;
}

bool equal_online_reply(OnlineReply a, OnlineReply b) {
    return (a.parity == b.parity);
}

string serialize_offline_reply(OfflineReply reply) {
    string res;
    interface::OfflineReply r;
    chrono::system_clock::time_point start, end;
    for (int i = 0; i < reply.parities.size(); i++) {
        Block blk = reply.parities[i];
        for (int j = 0; j < 16000/64; j++) {
            uint64_t result = 0;
            uint64_t mask = 1;
            for (int k = j*64; k < (j+1)*64; k++) {
                if (blk[k] == 1)
                    result |= mask;
                mask <<= 1;
            }
            r.add_parities(result);
        }
    }
    r.SerializeToString(&res);
    return res;
}

OfflineReply deserialize_offline_reply(string msg) {
    interface::OfflineReply r;
    if (!r.ParseFromString(msg)) {
        cout << "deserialize offline reply failed\n";
        assert(0);
    }
    OfflineReply reply;
    for (int i = 0; i < r.parities_size()/(16000/64); i++) {
        bitset<16000> bitsets(0);
        for (int j = 0; j < 16000/64; j++) {
            uint64_t val = r.parities()[i*(16000/64)+j];
            uint64_t mask = 1;
            for (int k = j*64; k < (j+1)*64; k++) {
                if ((val&mask) != 0)
                    bitsets[k] = 1;
                else
                    bitsets[k] = 0;
                mask <<= 1;
            }
        }
        reply.parities.push_back(bitsets);
    }
    return reply;
}

bool equal_offline_reply(OfflineReply a, OfflineReply b) {
    if (a.parities.size() != b.parities.size())
        return false;
    for (int i = 0; i < a.parities.size(); i++) {
        if (a.parities[i] != b.parities[i]) {
            // cout << a.parities[i] << endl;
            // cout << b.parities[i] << endl;
            return false;
        }
    }
    return true;
}

string serialize_online_query(OnlineQuery q) {
    interface::OnlineQuery query;
    for (int i = 0; i < q.indices.size(); i++) {
        query.add_indices(q.indices[i]);
    }
    string res;
    query.SerializeToString(&res);
    return res;
}

OnlineQuery deserialize_online_query(string msg) {
    interface::OnlineQuery query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize online query failed\n";
        assert(0);
    }
    OnlineQuery q;
    for (int i = 0; i < query.indices_size(); i++) {
        q.indices.push_back(query.indices()[i]);
    }
    return q;
}

bool equal_online_query(OnlineQuery a, OnlineQuery b) {
    if (a.indices.size() != b.indices.size())
        return false;
    for (int i = 0; i < a.indices.size(); i++) {
        if (a.indices[i] != b.indices[i])
            return false;
    }
    return true;
}

string serialize_offline_query(OfflineQuery q) {
    interface::OfflineQuery query;
    // check 
    for (int i = 0; i < q.sets.size(); i++) {
        interface::SetDesc* desc = query.add_sets();
        for (int j = 0; j < KeyLen; j++) {
            desc->add_sk(q.sets[i].sk[j]);
        }
        desc->set_shift(q.sets[i].shift);
        for (int j = 0; j < q.sets[i].aux.size(); j++) {
            desc->add_aux(get<0>(q.sets[i].aux[j]));
            desc->add_aux(get<1>(q.sets[i].aux[j]));
        }
    }
    string res;
    query.SerializeToString(&res);
    return res;
}

OfflineQuery deserialize_offline_query(string msg) {
    interface::OfflineQuery query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize offline query failed\n";
        assert(0);
    }
    OfflineQuery q;

    // check 
    for (int i = 0; i < query.sets_size(); i++) {
        SetDesc desc;
        for (int j = 0; j < KeyLen; j++) {
            desc.sk[j] = query.sets(i).sk(j);
        }
        desc.shift = query.sets(i).shift();
        for (int j = 0; j < query.sets(i).aux_size()/2; j++) {
            desc.aux.push_back(tuple<uint32_t, uint32_t>(query.sets(i).aux()[j*2], query.sets(i).aux()[j*2+1]));
        }
        q.sets.push_back(desc);
    }
    return q;
}

bool equal_offline_query(OfflineQuery a, OfflineQuery b) {
    if (a.sets.size() != b.sets.size()) {
        return false;
    }

    for (int i = 0; i < a.sets.size(); i++) {
        for (int j = 0; j < KeyLen; j++) {
            if (a.sets[i].sk[j] != b.sets[i].sk[j]) {
                return false;
            }
        }
        if (a.sets[i].shift != b.sets[i].shift)
            return false;
        if (a.sets[i].aux.size() != b.sets[i].aux.size()) {
            return false;
        }
        for (int j = 0; j < a.sets[i].aux.size(); j++) {
            if (a.sets[i].aux[j] != b.sets[i].aux[j]) {
                return false;
            }
        }
    }

    return true;
}

string serialize_offline_add_query(UpdateQueryAdd q) {
    interface::OfflineAddQueryShort query;
    query.set_nbrsets(q.nbrsets);
    query.set_setsize(q.setsize);
    for (int i = 0; i < q.diffsets.size(); i++) {
        interface::DiffSetDesc* info = query.add_diffsets();
        for (int j = 0; j < KeyLen; j++) {
            info->add_sk(q.diffsets[i].sk[j]);
        }
        
        info->set_shift(q.diffsets[i].shift);

        for (int j = 0; j < q.diffsets[i].aux_curr.size(); j++) {
            info->add_aux_curr(get<0>(q.diffsets[i].aux_curr[j]));
            info->add_aux_curr(get<1>(q.diffsets[i].aux_curr[j]));
        }
        for (int j = 0; j < q.diffsets[i].aux_prev.size(); j++) {
            info->add_aux_prev(get<0>(q.diffsets[i].aux_prev[j]));
            info->add_aux_prev(get<1>(q.diffsets[i].aux_prev[j]));
        }
    }

    for (int i = 0; i < KeyLen; i++) {
        query.add_mk(q.mk[i]);
    }

    string res;
    query.SerializeToString(&res);
    return res;
}

UpdateQueryAdd deserialize_offline_add_query(string msg) {
    
    interface::OfflineAddQueryShort query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize offline add query failed\n";
        assert(0);
    }
    UpdateQueryAdd q;
    q.nbrsets = query.nbrsets();
    q.setsize = query.setsize();

    for (int i = 0; i < query.diffsets_size(); i++) {
        DiffSetDesc info;
        for (int j = 0; j < KeyLen; j++) {
            info.sk[j] = query.diffsets(i).sk(j);
        }

        info.shift = query.diffsets(i).shift();

        for (int j = 0; j < query.diffsets()[i].aux_curr_size() / 2; j++) {
            std::tuple<uint32_t, uint32_t> t(query.diffsets()[i].aux_curr()[j*2], query.diffsets()[i].aux_curr()[j*2+1]);
            info.aux_curr.push_back(t);
        }
        for (int j = 0; j < query.diffsets()[i].aux_prev_size() / 2; j++) {
            std::tuple<uint32_t, uint32_t> t(query.diffsets()[i].aux_prev()[j*2], query.diffsets()[i].aux_prev()[j*2+1]);
            info.aux_prev.push_back(t);
        }
        q.diffsets.push_back(info);
    }

    for (int i = 0; i < KeyLen; i++) {
        q.mk[i] = query.mk(i);
    }
    return q;
}

bool equal_offline_add_query(UpdateQueryAdd a, UpdateQueryAdd b) {
    if (a.nbrsets != b.nbrsets || a.setsize != b.setsize || a.diffsets.size() != b.diffsets.size()) {
        return false;
    }
    for (int i = 0; i < KeyLen; i++) {
        if (a.mk[i] != b.mk[i])
            return false;
    }
    for (int i = 0; i < a.diffsets.size(); i++) {
        if (a.diffsets[i].shift != b.diffsets[i].shift || a.diffsets[i].aux_curr.size() != b.diffsets[i].aux_curr.size() || a.diffsets[i].aux_prev.size() != b.diffsets[i].aux_prev.size()) {
            return false;
        }
        for (int j = 0; j < KeyLen; j++) {
            if (a.diffsets[i].sk[j] != b.diffsets[i].sk[j]) {
                return false;
            }
        }
        for (int j = 0; j < a.diffsets[i].aux_curr.size(); j++) {
            if (a.diffsets[i].aux_curr[j] != b.diffsets[i].aux_curr[j]) {
                return false;
            }
        }
        for (int j = 0; j < a.diffsets[i].aux_prev.size(); j++) {
            if (a.diffsets[i].aux_prev[j] != b.diffsets[i].aux_prev[j]) {
                return false;
            }
        }
    }

    return true;
}

string serializeQuery(string& query, interface::QueryType type) {
    interface::Query q;
    q.set_type(type);
    q.set_msg(query);
    string res;
    q.SerializeToString(&res);
    return res;
}

Block generateRandBlock() {
    std::bitset<16000> blk; 
    for (int i = 0; i < 16000; i++) {
       blk[i] = rand() % 2; 
    }
    return blk;
}

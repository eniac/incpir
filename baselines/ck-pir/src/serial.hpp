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

#define KeyLen 16

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
    r.set_nbrsets(reply.nbrsets);
    chrono::system_clock::time_point start, end;
    for (int i = 0; i < reply.hints.size(); i++) {
        Block blk = reply.hints[i];
        for (int j = 0; j < 16000/64; j++) {
            uint64_t result = 0;
            uint64_t mask = 1;
            for (int k = j*64; k < (j+1)*64; k++) {
                if (blk[k] == 1)
                    result |= mask;
                mask <<= 1;
            }
            r.add_hints(result);
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
    reply.nbrsets = r.nbrsets();
    for (int i = 0; i < r.hints_size()/(16000/64); i++) {
        bitset<16000> bitsets(0);
        for (int j = 0; j < 16000/64; j++) {
            uint64_t val = r.hints()[i*(16000/64)+j];
            uint64_t mask = 1;
            for (int k = j*64; k < (j+1)*64; k++) {
                if ((val&mask) != 0)
                    bitsets[k] = 1;
                else
                    bitsets[k] = 0;
                mask <<= 1;
            }
        }
        reply.hints.push_back(bitsets);
    }
    return reply;
}

bool equal_offline_reply(OfflineReply a, OfflineReply b) {
    if ((a.nbrsets != b.nbrsets) || (a.hints.size() != b.hints.size()))
        return false;
    for (int i = 0; i < a.hints.size(); i++) {
        if (a.hints[i] != b.hints[i]) {
            return false;
        }
    }
    return true;
}

string serialize_online_query(OnlineQuery q) {
    interface::OnlineQuery query;
    query.set_height(q.height);
    query.set_bitvec(q.bitvec);
    

    for (int i = 0; i < q.keys.size(); i++) {
        for (int j = 0; j < KeyLen; j++) {
            query.add_keys(q.keys[i][j]);
        }
    }
    query.set_shifts(q.shift);
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
    q.height = query.height();
    q.bitvec = query.bitvec();
    q.shift = query.shifts();

    for (int i = 0; i < query.keys_size() / KeyLen; i++) {
        array<uint8_t, KeyLen> k;
        for (int j = 0; j < KeyLen; j++) {
            k[j] = ((uint8_t)query.keys()[i*KeyLen+j]);
        }
        q.keys.push_back(k);
    }
    return q;
}

bool equal_online_query(OnlineQuery a, OnlineQuery b) {
    if (a.keys.size() != b.keys.size() || a.height != b.height || a.bitvec != b.bitvec || a.shift != b.shift)
        return false;
    for (int i = 0; i < a.keys.size(); i++) {
        for (int j = 0; j < KeyLen; j++) {
            if (a.keys[i][j] != b.keys[i][j]) {
                cout << i << " " << j << endl;
                cout << a.keys[i][j] << " " << b.keys[i][j] << endl;
                return false;
            }
        }
    }
    return true;
}

string serialize_offline_query(OfflineQuery q) {
    interface::OfflineQuery query;
    query.set_nbrsets(q.nbrsets);
    query.set_setsize(q.setsize);

    // serialize offline keys, do not combine every 4 uint8 into uint32 for now
    assert(KeyLen % 4 == 0);
    for (int i = 0; i < q.offline_keys.size(); i++) {
	// for each key
	for (int j = 0; j < KeyLen/4; j++) {
            uint32_t tmp = 0;

            for (int b = 0; b < 4; b++) {
                tmp <<= 8;
                tmp |= q.offline_keys[i][4*j+b];
            }
            query.add_offline_keys(tmp);     
        }
    }

    for (int i = 0; i < q.shifts.size(); i++) {
        query.add_shifts(q.shifts[i]);
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
    q.nbrsets = query.nbrsets();
    q.setsize = query.setsize();

    for (int i = 0; i < (query.offline_keys_size()*4 / KeyLen); i++) {
        Key key;
	for (int j = 0; j < KeyLen/4; j++) {
            // for each j, uint32_t
            // parse to key
            uint32_t tmp = query.offline_keys()[i*(KeyLen/4)+j];
            key[4*j+0] = uint8_t((tmp>>24) & 0xFF);
            key[4*j+1] = uint8_t((tmp>>16) & 0xFF);
            key[4*j+2] = uint8_t((tmp>>8) & 0xFF);
            key[4*j+3] = uint8_t(tmp & 0xFF);
        }
        q.offline_keys.push_back(key);
    }

    for (int i = 0; i < query.shifts_size(); i++) {
        q.shifts.push_back(query.shifts()[i]);
    }
    return q;
}

bool equal_offline_query(OfflineQuery a, OfflineQuery b) {
    if (a.nbrsets != b.nbrsets || a.setsize != b.setsize || a.offline_keys.size() != b.offline_keys.size() || a.shifts.size() != b.shifts.size()) {
        return false;
    }

    for (int i = 0; i < a.offline_keys.size(); i++) {
        for (int j = 0; j < KeyLen; j++) {
            if (a.offline_keys[i][j] != b.offline_keys[i][j])
                return false;
        }
    }

    for (int i = 0; i < a.shifts.size(); i++) {
        if (a.shifts[i] != b.shifts[i])
            return false;
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
#include "interface.pb.h"
#include "dpf.h"
#include "hashdatastore.h"

#include <chrono>
#include <iostream>
#include <math.h>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

typedef struct {
    std::vector<uint8_t> left_query;
    std::vector<uint8_t> right_query;
} ClientQuery;

/*typedef struct {
    hashdatastore::hash_type reply;
} ServerReply;*/

using ServerReply = std::vector<uint32_t>;

class PIRClient {
public:
    PIRClient(uint32_t lgn_);
    PIRClient() {}

    uint32_t lgn;
    ClientQuery generate_query(int idx);

};

class PIRServer {
public:
    uint32_t lgn;
    hashdatastore *database = nullptr;

    PIRServer() {}
    PIRServer(uint32_t lgn_, hashdatastore *database_);

    ServerReply generate_left_reply(std::vector<uint8_t> left_query);
    ServerReply generate_right_reply(std::vector<uint8_t> right_query);
};

PIRClient::PIRClient(uint32_t lgn_) {
    lgn = lgn_;
    cout << "client created." << endl;

}

PIRServer::PIRServer(uint32_t lgn_, hashdatastore *database_) {

    if (database_ == nullptr) {
        throw std::invalid_argument("database cannot be null");
    }

    lgn = lgn_;
    database = database_;

    cout << "server created." << endl;

}

/*ServerReply PIRServer::generate_left_reply(std::vector<uint8_t> left_query) {
    ServerReply server_reply;

    std::vector<uint8_t> left_eval = DPF::EvalFull8(left_query, lgn);

    // Did this because benchmark need to be aligned
    // with 2kb item in PIR-Tor
    for (int i = 0; i < 62; i++)
        server_reply.reply = (*database).answer_pir2(left_eval);

    return server_reply;
}*/

ServerReply PIRServer::generate_left_reply(std::vector<uint8_t> left_query) {
    hashdatastore::hash_type server_reply;

    std::vector<uint8_t> left_eval = DPF::EvalFull8(left_query, lgn);

    // Did this because benchmark need to be aligned
    // with 2kb item in PIR-Tor
    vector<uint32_t> vec_reply;

    for (int i = 0; i < 62; i++) {
        server_reply = (*database).answer_pir2(left_eval);
        vector<uint32_t> v(8);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(v.data()), server_reply);

        for (int j = 0; j < 8; j++) {
            vec_reply.push_back(v[j]);
        }
    }

    return vec_reply;
}


/*ServerReply PIRServer::generate_right_reply(std::vector<uint8_t> right_query) {
    ServerReply server_reply;

    std::vector<uint8_t> right_eval = DPF::EvalFull8(right_query, lgn);

    // Did this because benchmark need to be aligned
    // with 2kb item in PIR-Tor
    for (int i = 0; i < 62; i++)
        server_reply.reply = (*database).answer_pir2(right_eval);

    return server_reply;
}*/

ServerReply PIRServer::generate_right_reply(std::vector<uint8_t> right_query) {

    hashdatastore::hash_type server_reply;

    std::vector<uint8_t> right_eval = DPF::EvalFull8(right_query, lgn);

    // Did this because benchmark need to be aligned
    // with 2kb item in PIR-Tor

    vector<uint32_t> vec_reply;

    for (int i = 0; i < 62; i++) {
        server_reply = (*database).answer_pir2(right_eval);
        vector<uint32_t> v(8);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(v.data()), server_reply);

        for (int j = 0; j < 8; j++) {
            vec_reply.push_back(v[j]);
        }
    }

    return vec_reply;
}

ClientQuery PIRClient::generate_query(int idx) {

    ClientQuery query;

    // generate DPF keys w.r.t. query index
    auto keys = DPF::Gen(idx, lgn);

    // two keys
    query.left_query = keys.first;
    query.right_query = keys.second;

    return query;
}

int test_baseline() {

    // set database
    size_t N = 13;
    hashdatastore store;
    store.reserve(1ULL << N);
    for (size_t i = 0; i < (1ULL << N); i++) {
        store.push_back(_mm256_set_epi64x(i, i, i, i));
    }

    // query index
    int qry_idx = 123;

    // generate DPF keys w.r.t. query index
    auto keys = DPF::Gen(qry_idx, N);

    // two keys
    auto a = keys.first;
    auto b = keys.second;

    // server answer
    std::vector<uint8_t> aaaa = DPF::EvalFull8(a, N);
    std::vector<uint8_t> bbbb = DPF::EvalFull8(b, N);

    hashdatastore::hash_type answerA = store.answer_pir2(aaaa);
    hashdatastore::hash_type answerB = store.answer_pir2(bbbb);

    hashdatastore::hash_type answer = _mm256_xor_si256(answerA, answerB);
    if (_mm256_extract_epi64(answer, 0) == qry_idx) {
        return 0;
    } else {
        std::cout << "PIR answer wrong\n";
        return -1;
    }

}

string serialize_query(ClientQuery q) {
    interface::query query;

    /*cout << "serial: left query size " << q.left_query.size() << endl;
    cout << "serial: right query size " << q.right_query.size() << endl;*/

    // TODO check change here
    for (int i = 0; i < q.left_query.size()/4 + 1; i++) {
        uint32_t tmp = 0;
        for (int b = 0; b < 4; b++) {
            tmp <<= 8;
            if (4*i + b < q.left_query.size())
                tmp |= q.left_query[4*i+b];
            else tmp |= 0xFF;
        }
        // if (i == 1) {
        //     printf("left serial:");
        //     printf("%x\n", tmp);
        // }

        query.add_left_query(tmp);
        //query.add_left_query((uint32_t)q.left_query[i]);
    }


    for (int i = 0; i < q.right_query.size()/4 + 1; i++) {
        uint32_t tmp = 0;
        for (int b = 0; b < 4; b++) {
            tmp <<= 8;
            if (4*i + b < q.right_query.size())
                tmp |= q.right_query[4*i+b];
            else tmp |= 0xFF;
        }
        // if (i == 1) {
        //     printf("right serial:");
        //     printf("%x\n", tmp);
        // }

        query.add_right_query(tmp);

        //query.add_right_query((uint32_t)q.right_query[i]);
    }
    string res;
    query.SerializeToString(&res);
    return res;
}

ClientQuery deserialize_query(string msg) {
    interface::query query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize query failed\n";
        assert(0);
    }
    ClientQuery q;
    // TOOD check change here
    uint32_t tmp = 0;

    for (int i = 0; i < query.left_query_size(); i++) {

        tmp = query.left_query()[i];

        uint8_t tail;
        if (i == query.left_query_size() - 1) {

            tail = ((tmp>>24) & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.left_query.push_back((uint8_t)(tail));
            }

            tail = ((tmp>>16) & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.left_query.push_back((uint8_t)(tail));
            }

            tail = ((tmp>>8) & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.left_query.push_back((uint8_t)(tail));
            }

            tail = (tmp & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.left_query.push_back((uint8_t)(tail));
            }
        }


        q.left_query.push_back((uint8_t)((tmp>>24) & 0xFF));
        q.left_query.push_back((uint8_t)((tmp>>16) & 0xFF));
        q.left_query.push_back((uint8_t)((tmp>>8) & 0xFF));
        q.left_query.push_back((uint8_t)(tmp & 0xFF));

        //q.left_query.push_back((uint8_t)(query.left_query()[i]));
    }

    // cout << "deserial: left query size " << q.left_query.size() << endl;

    for (int i = 0; i < query.right_query_size(); i++) {

        tmp = query.right_query()[i];

        uint8_t tail;
        if (i == query.right_query_size() - 1) {
            tail = ((tmp>>24) & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.right_query.push_back((uint8_t)(tail));
            }

            tail = ((tmp>>16) & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.right_query.push_back((uint8_t)(tail));
            }

            tail = ((tmp>>8) & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.right_query.push_back((uint8_t)(tail));
            }

            tail = (tmp & 0xFF);
            if (tail == 0xFF)
                break;
            else {
                q.right_query.push_back((uint8_t)(tail));
            }
        }


        q.right_query.push_back((uint8_t)((tmp>>24) & 0xFF));
        q.right_query.push_back((uint8_t)((tmp>>16) & 0xFF));
        q.right_query.push_back((uint8_t)((tmp>>8) & 0xFF));
        q.right_query.push_back((uint8_t)(tmp & 0xFF));


        //q.right_query.push_back((uint8_t)(query.right_query()[i]));
    }

    // cout << "deserial: right query size " << q.right_query.size() << endl;

    return q;
}

bool equal_query(ClientQuery a, ClientQuery b) {
    if (a.left_query.size() != b.left_query.size() || a.right_query.size() != b.right_query.size())
        return false;
    for (int i = 0; i < a.left_query.size(); i++) {
        if (a.left_query[i] != b.left_query[i])
            return false;
    }
    for (int i = 0; i < a.right_query.size(); i++) {
        if (a.right_query[i] != b.right_query[i])
            return false;
    }
    return true;
}

string serialize_reply(ServerReply l, ServerReply r) {
    interface::reply reply;
    for (int i = 0; i < l.size(); i++) {
        reply.add_v(l[i]);
    }
    for (int i = 0; i < r.size(); i++) {
        reply.add_v(r[i]);
    }
    // reply.add_v(_mm256_extract_epi32(l.reply, 0));
    // reply.add_v(_mm256_extract_epi32(l.reply, 1));
    // reply.add_v(_mm256_extract_epi32(l.reply, 2));
    // reply.add_v(_mm256_extract_epi32(l.reply, 3));
    // reply.add_v(_mm256_extract_epi32(l.reply, 4));
    // reply.add_v(_mm256_extract_epi32(l.reply, 5));
    // reply.add_v(_mm256_extract_epi32(l.reply, 6));
    // reply.add_v(_mm256_extract_epi32(l.reply, 7));

    // reply.add_v(_mm256_extract_epi32(r.reply, 0));
    // reply.add_v(_mm256_extract_epi32(r.reply, 1));
    // reply.add_v(_mm256_extract_epi32(r.reply, 2));
    // reply.add_v(_mm256_extract_epi32(r.reply, 3));
    // reply.add_v(_mm256_extract_epi32(r.reply, 4));
    // reply.add_v(_mm256_extract_epi32(r.reply, 5));
    // reply.add_v(_mm256_extract_epi32(r.reply, 6));
    // reply.add_v(_mm256_extract_epi32(r.reply, 7));
    string res;
    reply.SerializeToString(&res);
    return res;
}

void deserialize_reply(string msg, ServerReply& l, ServerReply& r) {
    interface::reply reply;
    if (!reply.ParseFromString(msg)) {
        cout << "deserialize reply failed\n";
        assert(0);
    }
    assert(reply.v_size() % 2 == 0);
    for (int i = 0; i < reply.v_size()/2; i++) {
        l.push_back(reply.v()[i]);
        r.push_back(reply.v()[i+reply.v_size()/2]);
    }
    // l.reply = _mm256_setr_epi32(reply.v()[0], reply.v()[1], reply.v()[2], reply.v()[3], reply.v()[4], reply.v()[5], reply.v()[6], reply.v()[7]);
    // r.reply = _mm256_setr_epi32(reply.v()[8], reply.v()[9], reply.v()[10], reply.v()[11], reply.v()[12], reply.v()[13], reply.v()[14], reply.v()[15]);
}

// bool equal_reply(ServerReply a, ServerReply b) {
//     return _mm256_cmpeq_epi64_mask(a.reply, b.reply);
// }

__m256i uint32_to_hash(const vector<uint32_t>& in) {
    assert(in.size() == 8);
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(in.data()));
}

vector<uint32_t> hash_uint32(__m256i in) {
    vector<uint32_t> out(8);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out.data()), in);
    return out;
}
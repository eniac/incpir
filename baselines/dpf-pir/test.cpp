
#include "serial.hpp"
#include <stdio.h>
#include <unistd.h>


int main(int argc, char** argv) {

    /*
     * Baseline: DPF-PIR
     * (This does not have preprocessing phase,
     * and requires two non-colluding servers)
     *
     * Client has an interested index i
     * Client generates two keys based on i: Gen(i)-> k1, k2
     * Client sends k1 to left server, and k2 to right server
     *
     * Left server gets k1
     * Left server computes Eval(k1) -> reply1
     *
     * Right server gets k2
     * Right server computes Eval(k2) -> reply2
     *
     * Client receives reply1 and reply2
     * Client recover the interested block: reply1 \xor reply2
     * */

    /*  setup database (database size = (1<<lgn), each item 256 bits) */
    int lgn = 16;
    int opt;
    while ((opt = getopt(argc, argv, "g:")) != -1) {
        if (opt == 'g') {
            lgn = atoi(optarg);
        }
    }

    hashdatastore *dbptr;
    hashdatastore store;
    store.reserve(1ULL << lgn);
    for (size_t i = 0; i < (1ULL << lgn); i++) {
        store.push_back(_mm256_set_epi64x(i, i, i, i));
    }
    dbptr = &store;

    // setup client and server (actually two servers)
    PIRClient client(lgn);
    PIRServer server(lgn, dbptr);

    // client generates query
    int qry_idx = rand() % (1ULL << lgn);
    auto qry_st = chrono::high_resolution_clock::now();

    ClientQuery query = client.generate_query(qry_idx);

    ClientQuery query1 = deserialize_query(serialize_query(query));
    assert(equal_query(query, query1));

    cout << "client query size: "
       << double(serialize_query(query).size())/double(1000) << " KB" << endl;

    auto qry_ed = chrono::high_resolution_clock::now();
    auto qry_time = chrono::duration_cast<chrono::microseconds>
            (qry_ed-qry_st).count();
    cout << "client query generated in " << qry_time << " microsec." << endl;


    // server generates reply 
    auto st = chrono::high_resolution_clock::now();

    ServerReply left_reply = server.generate_left_reply(query1.left_query);
    ServerReply right_reply = server.generate_right_reply(query1.right_query);

    string msg = serialize_reply(left_reply, right_reply);
    // cout << "reply size: " << msg.size() << endl;
    ServerReply l, r;
    deserialize_reply(msg, l, r);

    hashdatastore::hash_type left_hash_reply, right_hash_reply;

    vector<uint32_t> left_vec(l.begin(), l.begin()+8);
    vector<uint32_t> right_vec(r.begin(), r.begin()+8);

    left_hash_reply = uint32_to_hash(left_vec);
    right_hash_reply = uint32_to_hash(right_vec);

    // client recovers the block 
    hashdatastore::hash_type answer = _mm256_xor_si256(left_hash_reply, right_hash_reply);

    if (_mm256_extract_epi64(answer, 0) == qry_idx) {
        cout << "PIR answer correct\n";
    } else {
        cout << "PIR answer wrong\n";
    }

    return 0;
}
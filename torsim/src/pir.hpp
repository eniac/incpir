#ifndef _PIRBASIC
#define _PIRBASIC

#include <vector>
#include <bitset>
#include <tuple>
#include <array>

#define KeyLen 16

#define ONESEC 1000000
#define ONEMS 1000
#define ONEKB 8000

#define BlockLen 16000

using namespace std;

typedef array<uint8_t, KeyLen> Key;  // key length: 128 bits

typedef bitset<BlockLen> Block;
typedef vector<Block> Database;

typedef struct {
    uint32_t dbrange;
    uint32_t setsize;
    uint32_t replica;
    uint32_t nbrsets;
} SetupParams;

typedef struct {
    Key sk;                                    // set key
    uint32_t shift;                            // shift for 1st range
    vector<tuple<uint32_t, uint32_t> > aux;    // aux list
} SetDesc;
// 128 bits + 32 bits + 64 bits * aux.size

typedef struct {
    vector<SetDesc> sets;
    vector<Block> parities;
} LocalHints;

typedef struct {
    // each element in [sets] is a set description
    // with key, shift, and aux
    vector<SetDesc> sets;
} OfflineQuery;

typedef struct {
    std::vector<Block> parities;
} OfflineReply;

typedef struct {
    std::vector<uint32_t> indices;
} OnlineQuery;

typedef struct {
    Block parity;
} OnlineReply;

// three update queries during offline phase
typedef struct {
    Key sk;
    uint32_t shift;

    // use two auxs, but the range list from one of them can be removed (since they are same)
    // keep this for now, see if simplify later
    vector<tuple<uint32_t, uint32_t> > aux_prev;
    vector<tuple<uint32_t, uint32_t> > aux_curr;

    // aux_curr should be one unit longer than aux_prev
    // for each tuple in aux_prev, Eval on points in (t_curr, t_prev]
} DiffSetDesc;

typedef struct {
    uint32_t nbrsets;
    uint32_t setsize;
    Key mk;
   vector<DiffSetDesc> diffsets;
} UpdateQueryAdd;

typedef struct {
    uint32_t nbrsets;
    uint32_t setsize;
} UpdateQueryEdit;

typedef struct {
    uint32_t nbrsets;
    uint32_t setsize;
} UpdateQueryDelete;

Key kdf(Key mk, Key sk, uint32_t batch_no);

#endif

#ifndef _PIRBASIC
#define _PIRBASIC

#include <iostream>
#include <set>
#include <vector>
#include "puncprf.hpp"

#define KeyLen 16

typedef unsigned long long ulonglong;

typedef std::array<uint8_t, KeyLen> Key;
typedef std::bitset<16000> Block;
typedef std::vector<Block> Database;

typedef struct {
    Key prf_key;
    uint32_t shift = 0;
    Block hint;
} SetDesc;

typedef struct {
    uint32_t nbrsets;
    uint32_t setsize;
    std::vector<Key> offline_keys;
    std::vector<uint32_t> shifts;
} OfflineQuery;

typedef struct {
    int height;
    uint32_t bitvec = 0; // starting from right most bit
    std::vector<Key> keys;
    uint32_t shift;
} OnlineQuery;
// actually PuncKeys

typedef struct {
    uint32_t nbrsets;
    std::vector<Block> hints;
} OfflineReply;

typedef struct {
    Block parity;
} OnlineReply;

Block generateRandBlock();

#endif



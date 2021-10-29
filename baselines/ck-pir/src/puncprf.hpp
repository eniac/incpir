#pragma once
#ifndef _PUNCPRF
#define _PUNCPRF

#include <vector>
#include <bitset>
#include <set>
#include <array>
#include <tuple>
#include <cstring>

#include "pir.hpp"

using namespace std;

#define KeyLen 16
typedef std::array<uint8_t, KeyLen> Key;

typedef struct {
    int height;
    uint32_t bitvec = 0; // starting from right most bit
    vector<Key> keys;
} PuncKeys;

void print_key(uint8_t *key);

tuple<Key, Key> PRG (Key key);

vector<uint32_t> BreadthEval(Key rootkey, int low, int high,
        uint32_t lgn, uint32_t range);

uint32_t Eval (Key key, uint32_t x, uint32_t lgn, uint32_t range);

int EvalPunc (PuncKeys punc_keys, uint32_t x, uint32_t lgn, uint32_t range);

PuncKeys Punc(Key key, uint32_t punc_x, uint32_t lgn);

#endif



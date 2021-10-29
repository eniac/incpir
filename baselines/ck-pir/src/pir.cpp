#include "pir.hpp"

Block generateRandBlock() {
    std::bitset<16000> blk; 
    for (int i = 0; i < 16000; i++) {
       blk[i] = rand() % 2; 
    }
    return blk;
}
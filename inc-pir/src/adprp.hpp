#ifndef _ADPRP
#define _ADPRP

#include "pir.hpp"
#include <bitset>
#include <stdint.h>

#define ROUNDS 7

uint32_t round_func(uint16_t block, Key key, uint16_t tweak,
                    uint32_t input_length, uint32_t output_length);

uint32_t feistel_prp(uint32_t block, uint32_t block_length,
                     uint32_t rounds, Key key);
uint32_t feistel_inv_prp(uint32_t block, uint32_t block_length,
                         uint32_t rounds, Key key);

uint32_t cycle_walk(uint32_t num, uint32_t range, Key key);
uint32_t inv_cycle_walk(uint32_t num, uint32_t range, Key key);


#endif

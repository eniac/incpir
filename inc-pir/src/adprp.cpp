#include "adprp.hpp"

#include <openssl/evp.h>

#include <cmath>
#include <bitset>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


uint32_t round_func(uint16_t block, Key key, uint16_t tweak,
                    uint32_t input_length, uint32_t output_length) {

    block = block ^ tweak;

    uint8_t *keyptr;
    keyptr = static_cast<uint8_t *>(malloc(KeyLen));
    memset(keyptr, 0, KeyLen);
    for (int i = 0; i < KeyLen; i++)
        keyptr[i] = key[i];


    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(KeyLen));
    memset(plaintext, 0, KeyLen);

    plaintext[KeyLen-2] = (uint8_t) ((block>>8) & 0xFF);
    plaintext[KeyLen-1] = (uint8_t) (block & 0xFF);

    int outlen;
    uint8_t outbuf[16];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyptr, NULL);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, KeyLen);

    uint32_t res = outbuf[KeyLen-2];
    res = res << 8;
    res = res | outbuf[KeyLen-1];
    res = res & ((1<<output_length)-1);

    free(plaintext);
    EVP_CIPHER_CTX_free(ctx);

    return res;
}


/**
 * @param block: input of PRP, of length at most 32 bits
 * @param block_length: input block length n, where n is the smallest number s.t. 2^n > PRP range
 * @param rounds: feistel network rounds
 * @param keys: subkeys array
 * @return
 */
uint32_t feistel_prp(uint32_t block, uint32_t block_length,
                     uint32_t rounds, Key key) {

    uint32_t left_length = block_length / 2;
    uint32_t right_length =  block_length - left_length;

    // split left and right
    uint32_t left = (block>>right_length) & ((1<<left_length)-1);
    uint32_t right = ((1<<right_length)-1) & block;

    uint32_t left1, right1;
    uint32_t perm_block;

    for (int i = 0; i < rounds; i++) {

        left1 = right;
        right1 = left ^ round_func(right, key, i+1, right_length, left_length);

        // concat left and right

        // re-assign left and right

        if (i == rounds - 1) {
            perm_block = (left1<<left_length) | right1;
        } else {
            perm_block = (left1<<left_length) | right1;

            left = perm_block>>right_length & ((1<<left_length)-1);
            right = perm_block & ((1<<right_length )- 1);
        }
    }

    return perm_block;
}


uint32_t feistel_inv_prp(uint32_t perm_block, uint32_t block_length,
                         uint32_t rounds, Key key) {

    uint32_t right_length = block_length / 2;
    uint32_t left_length = block_length - right_length;

    uint32_t right = perm_block & ((1<<right_length)-1);
    uint32_t left = (perm_block>>right_length) & ((1<<left_length)-1);

    uint32_t left1, right1;
    uint32_t block;

    for (int i = 0; i < rounds; i++) {

        right1 = left;
        left1 = right ^ round_func(left, key, rounds-i, left_length, right_length);

        if (i == rounds - 1) {
            block = (left1<<left_length) | right1;

        } else {
            block = (left1<<left_length) | right1;

            left = (block>>right_length) & ((1<<left_length)-1);
            right = block & ((1<<right_length)-1);
        }
    }

    return block;
}


uint32_t cycle_walk(uint32_t num, uint32_t range, Key key) {

    if (num >= range) {
        std::cout << "error:" << num << " , " << range << std::endl;
        throw std::invalid_argument("PRP input is invalid");
    }
    // compute the smallest n s.t. 2^n>range

    uint32_t cnt = log2(range) + 1;

    uint32_t tmp = feistel_prp(num, cnt, ROUNDS, key);

    while(tmp >= range) {
        tmp = feistel_prp(tmp, cnt, ROUNDS, key);
    }

    return tmp;
}


uint32_t inv_cycle_walk(uint32_t num, uint32_t range, Key key) {

    if (num >= range) {
        std::cout << "error:" << num << " , " << range << std::endl;
        throw std::invalid_argument("PRP input is invalid");
    }

    uint32_t cnt = log2(range) + 1;

    uint32_t tmp = feistel_inv_prp(num, cnt, ROUNDS, key);

    while(tmp >= range) {
        tmp = feistel_inv_prp(tmp, cnt, ROUNDS, key);
    }

    return tmp;
}

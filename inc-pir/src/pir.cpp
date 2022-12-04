#include "pir.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

using namespace std;


Key kdf(Key mk, Key sk, uint32_t batch_no) {

    uint8_t *mkptr = static_cast<uint8_t *>(malloc(KeyLen));
    for (int i = 0; i < KeyLen; i++) {
        mkptr[i] = mk[i];
    }
    uint8_t *skptr = static_cast<uint8_t *>(malloc(KeyLen));
    for (int i = 0; i < KeyLen; i++) {
        skptr[i] = sk[i];
    }

    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(KeyLen));
    memset(plaintext, 0, KeyLen);

    memcpy(plaintext, skptr, KeyLen);
    plaintext[0] ^= (batch_no & 0xFF);

    int outlen;
    uint8_t outbuf[16];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, mkptr, NULL);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, KeyLen);

    Key reskey;
    for (int i = 0; i < KeyLen; i++) {
        reskey[i] = outbuf[i];
    }

    EVP_CIPHER_CTX_free(ctx);
    free(plaintext);
    free(skptr);
    free(mkptr);

    return reskey;
}



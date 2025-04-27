#ifndef RSA_H
#define RSA_H
#include "BigInt.h"

typedef struct { BigInt *n, *exp; } RSAKey;

void rsa_generate_keypair(RSAKey *pub, RSAKey *priv, size_t bits);
void rsa_encrypt(const BigInt *m,const RSAKey *pub ,BigInt **c);
void rsa_decrypt(const BigInt *c,const RSAKey *priv,BigInt **m);

bool rsa_save_key(const char *file,const RSAKey *k,const char *label);
bool rsa_load_key(const char *file,RSAKey *k);
void rsa_free_key(RSAKey *k);

#endif

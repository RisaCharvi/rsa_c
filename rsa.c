
#include "rsa.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 

//Hardcoded P, Q, and E (hard to generate big valid primes)
static const uint64_t P_VAL = 4294967311ULL;   /* 0x10000000F (33 bits) */
static const uint64_t Q_VAL = 4294967357ULL;   /* 0x10000003D (33 bits) */
static const uint64_t E_VAL = 65537;           /* public exponent       */

void rsa_generate_keypair(RSAKey *pub, RSAKey *priv, size_t bits_unused)
{
    (void)bits_unused;                    

    BigInt *p_bi       = bi_from_u64(P_VAL);
    BigInt *q_bi       = bi_from_u64(Q_VAL);
    BigInt *p_minus_1  = bi_from_u64(P_VAL - 1);
    BigInt *q_minus_1  = bi_from_u64(Q_VAL - 1);
    BigInt *e_bi       = bi_from_u64(E_VAL);
    BigInt *n_bi       = NULL;
    BigInt *phi_bi     = NULL;
    BigInt *d_bi       = NULL;

    if (!p_bi || !q_bi || !p_minus_1 || !q_minus_1 || !e_bi) {
        fprintf(stderr, "Error creating BigInts for key generation constants.\n");
        bi_free(p_bi); bi_free(q_bi); bi_free(p_minus_1); bi_free(q_minus_1);
        bi_free(e_bi);
        exit(1);
    }

    // Calculate n = P * Q using BigInt multiplication
    bi_mul(p_bi, q_bi, &n_bi);
    if (!n_bi) { fprintf(stderr,"bi_mul failed for n\n"); exit(1); }

    // Calculate phi = (P-1) * (Q-1) using BigInt multiplication
    bi_mul(p_minus_1, q_minus_1, &phi_bi);
    if (!phi_bi) { fprintf(stderr,"bi_mul failed for phi\n"); exit(1); }


    // Calculate d = e^-1 mod phi using BigInt modular inverse
    if (!bi_modinv(e_bi, phi_bi, &d_bi)) {
         fprintf(stderr, "Error: Modular inverse calculation failed.\n");
         bi_free(p_bi); bi_free(q_bi); bi_free(p_minus_1); bi_free(q_minus_1);
         bi_free(e_bi); bi_free(n_bi); bi_free(phi_bi);
         exit(1);
    }
    


    
    pub->n   = n_bi;          // pub owns n_bi now
    pub->exp = e_bi;          // pub owns e_bi now

    priv->n   = bi_copy(n_bi); // priv needs its own copy of n
    priv->exp = d_bi;          // priv owns d_bi now
    if (!priv->n) { fprintf(stderr,"bi_copy failed for priv->n\n"); exit(1); }

 
    bi_free(p_bi);
    bi_free(q_bi);
    bi_free(p_minus_1);
    bi_free(q_minus_1);
    bi_free(phi_bi);

}


void rsa_encrypt(const BigInt *m, const RSAKey *pub,  BigInt **c)
{ bi_modexp(m, pub ->exp, pub ->n, c); }

void rsa_decrypt(const BigInt *c, const RSAKey *priv, BigInt **m)
{ bi_modexp(c, priv->exp, priv->n, m); }

bool rsa_save_key(const char *file, const RSAKey *k, const char *lbl)
{
    
    if (!k || !k->n || !k->exp) {
        fprintf(stderr, "Error: Attempting to save invalid key to %s.\n", file);
        return false;
    }

    FILE *f = fopen(file, "w");
    if (!f) {
        perror(file);
        return false;
    }

    fprintf(f, "-----BEGIN RSA %s KEY-----\n", lbl);
    if (!bi_write_hex(f, k->n)) {
         fprintf(stderr, "Error writing n to key file %s\n", file);
         fclose(f);
         return false;
    }
    if (!bi_write_hex(f, k->exp)) {
         fprintf(stderr, "Error writing exponent to key file %s\n", file);
         fclose(f);
         return false;
    }
    fprintf(f, "-----END RSA %s KEY-----\n", lbl);

    return !fclose(f); 
}

bool rsa_load_key(const char *file, RSAKey *k)
{
    FILE *f = fopen(file, "r");
    if (!f) { perror(file); return false; }

    char dummy[64];
    if (!fgets(dummy, sizeof dummy, f)) { 
        fprintf(stderr, "Error reading header from key file %s\n", file);
        fclose(f); return false;
    }

    k->n   = bi_read_hex(f);
    if (!k->n) {
         fprintf(stderr, "Error reading n from key file %s\n", file);
         fclose(f); return false;
    }

    k->exp = bi_read_hex(f);
     if (!k->exp) {
         fprintf(stderr, "Error reading exponent from key file %s\n", file);
         bi_free(k->n); 
         k->n = NULL;
         fclose(f); return false;
    }

    
    fgets(dummy, sizeof dummy, f);

    fclose(f);
    return true; 
}

void rsa_free_key(RSAKey *k)
{
    if (!k) return;
    bi_free(k->n);
    bi_free(k->exp);
    k->n = NULL;
    k->exp = NULL;
}

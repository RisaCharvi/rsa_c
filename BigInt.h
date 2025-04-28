
#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h> 

typedef struct {
    size_t   len;      //number of limbs
    uint32_t *limbs;   
} BigInt;


BigInt *bi_new(size_t len);              
void     bi_free(BigInt *n);             
BigInt  *bi_from_u64(uint64_t v);        
BigInt  *bi_copy(const BigInt *src);     
void     bi_trim(BigInt *n);             
size_t   bi_bitlen(const BigInt *n);     
int      bi_cmp(const BigInt *a, const BigInt *b);        

void bi_add(const BigInt *a, const BigInt *b, BigInt **res);      
void bi_sub(const BigInt *a, const BigInt *b, BigInt **res);      // a must be greater than b
void bi_mul(const BigInt *a, const BigInt *b, BigInt **res);      
void bi_mod(const BigInt *a, const BigInt *m, BigInt **res);      

void bi_divmod(const BigInt *a, const BigInt *m, BigInt **q_res, BigInt **r_res);
void bi_modexp(const BigInt *base, const BigInt *exp, const BigInt *mod,  BigInt **res);                    
void bi_gcd(const BigInt *a, const BigInt *b, BigInt **res);      
bool bi_modinv(const BigInt *a, const BigInt *m, BigInt **inv);// inv(a) mod m

void    bi_print_hex(const BigInt *n);                 
bool    bi_write_hex(FILE *fp, const BigInt *n);      
BigInt *bi_read_hex (FILE *fp);                        
#endif 
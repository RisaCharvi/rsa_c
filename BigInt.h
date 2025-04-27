
#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h> 

typedef struct {
    size_t   len;      /* number of limbs used (≥ 1)                */
    uint32_t *limbs;   /* little-endian limbs (LSW = limbs[0])      */
} BigInt;


BigInt *bi_new(size_t len);              /* allocate zero             */
void     bi_free(BigInt *n);             /* free (NULL-safe)          */
BigInt  *bi_from_u64(uint64_t v);        /* ctor from 64-bit constant */
BigInt  *bi_copy(const BigInt *src);     /* deep copy                 */
void     bi_trim(BigInt *n);             /* strip leading zero limbs  */
size_t   bi_bitlen(const BigInt *n);     /* # of significant bits     */
int      bi_cmp(const BigInt *a,
                const BigInt *b);        /* -1 / 0 / +1               */

void bi_add    (const BigInt *a, const BigInt *b, BigInt **res);      /* res=a+b   */
void bi_sub    (const BigInt *a, const BigInt *b, BigInt **res);      /* a≥b       */
void bi_mul    (const BigInt *a, const BigInt *b, BigInt **res);      /* res=a⋅b   */
void bi_mod    (const BigInt *a, const BigInt *m, BigInt **res);      /* res=a mod m */
// Calculates q = floor(a / m), r = a mod m
void bi_divmod (const BigInt *a, const BigInt *m, BigInt **q_res, BigInt **r_res);
void bi_modexp (const BigInt *base, const BigInt *exp,
                const BigInt *mod,  BigInt **res);                    /* res=base^exp mod mod */
void bi_gcd    (const BigInt *a, const BigInt *b, BigInt **res);      /* gcd       */
// Returns true on success (inv found), false otherwise. *inv is set.
bool bi_modinv (const BigInt *a, const BigInt *m, BigInt **inv);      /* a⁻¹ mod m */

void    bi_print_hex(const BigInt *n);                 /* printf-style */
bool    bi_write_hex(FILE *fp, const BigInt *n);       /* one-line hex */
BigInt *bi_read_hex (FILE *fp);                        /* read hex     */
#endif /* BITFIELD_H */

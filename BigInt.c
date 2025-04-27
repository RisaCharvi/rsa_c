
#include "BigInt.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> 
#include <ctype.h>

                                                   
static BigInt *bi_shift_left_bits(const BigInt *n, size_t k);
static void    bi_sub_inplace    (BigInt *acc, const BigInt *b);

// memory helpers
BigInt *bi_new(size_t len)
{
    BigInt *n = malloc(sizeof *n);
    if (!n) return NULL; // Check malloc success
    n->len   = (len ? len : 1);
    n->limbs = calloc(n->len, sizeof(uint32_t));
    if (!n->limbs) { // Check calloc success
        free(n);
        return NULL;
    }
    return n;
}

void bi_free(BigInt *n)
{
    if (!n) return;
    free(n->limbs);
    free(n);
}


BigInt *bi_from_u64(uint64_t v)
{
    // Handles only up to 64 bits correctly.
    // If v requires > 64 bits, this will truncate.
    size_t limbs_needed = 1;
    if (v > 0xFFFFFFFFULL) {
        limbs_needed = 2;
    }
    BigInt *n = bi_new(limbs_needed);
    if (!n) return NULL;

    n->limbs[0] = (uint32_t) v;
    if (n->len == 2) { // Use n->len which reflects actual allocation
         n->limbs[1] = (uint32_t)(v >> 32);
    }
    bi_trim(n); // Trim if upper limb became zero
    return n;
}


BigInt *bi_copy(const BigInt *src)
{
    if (!src) return NULL;
    BigInt *dst = bi_new(src->len);
    if (!dst) return NULL;
    memcpy(dst->limbs, src->limbs, src->len * sizeof(uint32_t));
   
    return dst;
}


void bi_trim(BigInt *n)
{
    
    if (!n || !n->limbs) return;
    // Reduce length while len > 1 and the most significant limb is zero
    while (n->len > 1 && n->limbs[n->len - 1] == 0) {
        --n->len;
    }
}


size_t bi_bitlen(const BigInt *n)
{
    
    if (!n || !n->limbs || n->len == 0) return 0; 

    // Find the most significant limb index that is non-zero
    size_t top_limb_idx = n->len - 1;
    while (top_limb_idx > 0 && n->limbs[top_limb_idx] == 0) {
        top_limb_idx--;
    }

    uint32_t msw = n->limbs[top_limb_idx];
    size_t bits_in_lower_limbs = 32 * top_limb_idx;
    size_t bits_in_msw = 0;

    // Calculate bits in the most significant non-zero limb
    if (msw == 0) {
        return 1; // Represent 0 as having bit length 1
    } else {
        uint32_t temp = msw;
        while (temp > 0) {
            bits_in_msw++;
            temp >>= 1;
        }
    }

    return bits_in_lower_limbs + bits_in_msw;
}


int bi_cmp(const BigInt *a, const BigInt *b)
{
    
    if (!a || !b) return 0; 
    if (a->len > b->len) return 1;
    if (a->len < b->len) return -1;
    // Lengths are equal, compare from most significant limb down
    for (size_t i = a->len; i-- > 0;) {
        if (a->limbs[i] > b->limbs[i]) return 1;
        if (a->limbs[i] < b->limbs[i]) return -1;
    }
    return 0; // They are equal
}


void bi_add(const BigInt *a, const BigInt *b, BigInt **res)
{
    size_t max_len = (a->len > b->len) ? a->len : b->len;
    // Result might need one extra limb for carry
    BigInt *r = bi_new(max_len + 1);
    if (!r) { *res = NULL; return; } 

    uint64_t carry = 0;
    size_t i = 0;
    while (i < max_len || carry > 0) {
         // Ensure we don't write past allocated space in r
         if (i >= r->len) {
             size_t new_len = r->len + 1;
             uint32_t *new_limbs = realloc(r->limbs, new_len * sizeof(uint32_t));
             if (!new_limbs) {
                 fprintf(stderr, "Error: Reallocation failed in bi_add.\n");
                 bi_free(r);
                 *res = NULL;
                 return;
             }
             r->limbs = new_limbs;
             r->limbs[r->len] = 0; // Initialize new limb
             r->len = new_len;
         }

        uint64_t term_a = (i < a->len) ? a->limbs[i] : 0;
        uint64_t term_b = (i < b->len) ? b->limbs[i] : 0;
        uint64_t sum = term_a + term_b + carry;

        r->limbs[i] = (uint32_t)sum; // Lower 32 bits
        carry = sum >> 32;          // Upper 32 bits (the carry)
        i++;
    }

    
    r->len = i; // i is now the number of limbs written

    bi_trim(r); 
    *res = r;   
}


static void bi_sub_inplace(BigInt *acc, const BigInt *b)
{
    // Assumes bi_cmp(acc, b) >= 0 has been checked by caller if necessary
    uint64_t borrow = 0;
    size_t max_len = acc->len; 

    for (size_t i = 0; i < max_len; ++i) {
        uint64_t term_acc = acc->limbs[i];
        uint64_t term_b = (i < b->len) ? b->limbs[i] : 0;

        
        uint64_t diff = term_acc - term_b - borrow;

        acc->limbs[i] = (uint32_t)diff; // Lower 32 bits

        // Determine next borrow: 1 if diff was negative, 0 otherwise
        // Check the 64th bit (sign bit if interpreted as signed)
        borrow = (diff >> 63) & 1;
    }
    // If final borrow is non-zero, it implies acc < b, which violates preconditions

    bi_trim(acc); // Remove any leading zero limbs created by subtraction
}


void bi_sub(const BigInt *a, const BigInt *b, BigInt **res)
{
    // Ensure a >= b as per function contract
    assert(bi_cmp(a,b) >= 0 && "bi_sub needs a ≥ b");

    
    BigInt *r = bi_copy(a);
    if (!r) { *res = NULL; return; } 

    
    bi_sub_inplace(r, b);

    *res = r; 
}



void bi_mul(const BigInt *a, const BigInt *b, BigInt **res)
{
    // Result can have up to a->len + b->len limbs
    size_t res_len = a->len + b->len;
    BigInt *r = bi_new(res_len);
    if (!r) { *res = NULL; return; } 

    for (size_t i = 0; i < a->len; ++i) {
        uint64_t carry = 0;
        // Don't compute if a limb is zero
        if (a->limbs[i] == 0) continue;

        for (size_t j = 0; j < b->len; ++j) {
            // Product of two limbs + existing value in result + carry from previous step
            uint64_t prod = (uint64_t)a->limbs[i] * b->limbs[j]
                           + r->limbs[i+j] + carry;

            r->limbs[i+j] = (uint32_t)prod; // Lower 32 bits
            carry = prod >> 32;          // Upper 32 bits (carry)
        }
        // Add final carry to the next limb position
        // This position might already have a value from the outer loop's previous iteration
        if (carry > 0) {
            // Propagate carry if necessary (handle potential chain reaction of carries)
            size_t k = i + b->len;
            while (k < r->len && carry > 0) {
                 uint64_t sum = (uint64_t)r->limbs[k] + carry;
                 r->limbs[k] = (uint32_t)sum;
                 carry = sum >> 32;
                 k++;
            }
            // Assert if carry still exists beyond allocated length (shouldn't happen)
            assert(carry == 0 && "Carry propagation exceeded allocated length in bi_mul");
        }
    }
    bi_trim(r); 
    *res = r;   
}



// Calculates q = floor(a / m), r = a mod m                         
void bi_divmod(const BigInt *a, const BigInt *m, BigInt **q_res, BigInt **r_res)
{
    // Initialize quotient and remainder
    BigInt *q = bi_from_u64(0);
    BigInt *r = bi_copy(a);
    BigInt *zero = bi_from_u64(0);
    BigInt *one = bi_from_u64(1); // Needed for divisor check
    BigInt *d = NULL;             // Intermediate divisor m << shift
    BigInt *q_term = NULL;        // Intermediate quotient term 2^shift
    BigInt *shifted_q_term = NULL;// Intermediate quotient term 2^shift (after shifting)
    BigInt *new_q = NULL;         // Intermediate quotient sum q + q_term


    // Handle edge cases and allocation failures
    if (!q || !r || !zero || !one) {
        fprintf(stderr, "Error: Allocation failed in bi_divmod setup.\n");
        goto divmod_error_cleanup;
    }
    // Modulus (divisor m) must be > 0
    if (bi_cmp(m, zero) <= 0) {
        fprintf(stderr, "Error: Divisor must be > 0 in bi_divmod.\n");
        goto divmod_error_cleanup;
    }

    // If a < m, then q=0, r=a
    if (bi_cmp(a, m) < 0) {
        // q is already 0 from bi_from_u64
        if (q_res) *q_res = q; else bi_free(q); q = NULL; 
        if (r_res) *r_res = r; else bi_free(r); r = NULL; 
        bi_free(zero); bi_free(one);
        return;
    }

    // Main division loop 
    while (bi_cmp(r, m) >= 0) {
        size_t r_bits = bi_bitlen(r);
        size_t m_bits = bi_bitlen(m);
        if (r_bits < m_bits) break; 

        size_t shift = r_bits - m_bits;
        d = bi_shift_left_bits(m, shift); // d = m << shift
        if (!d) goto divmod_error_cleanup;

        if (bi_cmp(r, d) < 0) {
            bi_free(d); d = NULL;
            if (shift == 0) {
                fprintf(stderr, "Internal error in bi_divmod shift calculation (shift=0).\n");
                goto divmod_error_cleanup; 
            }
            shift--;
            d = bi_shift_left_bits(m, shift);
            if (!d) goto divmod_error_cleanup;
        }

        // Subtract d from remainder r
        assert(bi_cmp(r, d) >= 0 && "Remainder should be >= adjusted divisor in bi_divmod");
        bi_sub_inplace(r, d);
        bi_free(d); d = NULL;

        // Add 2^shift to the quotient q
        q_term = bi_from_u64(1);
        if (!q_term) goto divmod_error_cleanup;
        if (shift > 0) {
            shifted_q_term = bi_shift_left_bits(q_term, shift);
             if (!shifted_q_term) goto divmod_error_cleanup;
            bi_free(q_term); q_term = NULL;
            q_term = shifted_q_term; shifted_q_term = NULL; 
        }
        // q = q + q_term
        new_q = NULL;
        bi_add(q, q_term, &new_q);
        if (!new_q) goto divmod_error_cleanup;
        bi_free(q); bi_free(q_term); q_term = NULL;
        q = new_q; new_q = NULL; 
    }

    
    bi_free(zero); bi_free(one);
    if (q_res) *q_res = q; else bi_free(q); 
    if (r_res) *r_res = r; else bi_free(r); 
    return;

divmod_error_cleanup:
    
    fprintf(stderr, "Error during bi_divmod calculation.\n");
    bi_free(q); bi_free(r); bi_free(zero); bi_free(one);
    bi_free(d); bi_free(q_term); bi_free(shifted_q_term); bi_free(new_q);
    if (q_res) *q_res = NULL; 
    if (r_res) *r_res = NULL;
}



static BigInt *bi_shift_left_bits(const BigInt *n, size_t k)
{
    size_t limb_shift = k / 32; // Number of full limbs to shift
    size_t bit_shift  = k % 32; // Number of bits to shift within limbs

    // Calculate required length for result: original + full shifts + 1 for potential carry/partial shift
    size_t res_len = n->len + limb_shift + (bit_shift > 0 ? 1 : 0);
    if (res_len == 0) res_len = 1; 
    BigInt *r = bi_new(res_len);
     if (!r) return NULL;

    if (bit_shift == 0) {
        for (size_t i = 0; i < n->len; ++i) {
             if (i + limb_shift < r->len) {
                r->limbs[i + limb_shift] = n->limbs[i];
             } else {
                 fprintf(stderr, "Error: Overflow in bi_shift_left_bits (limb shift).\n");
                 bi_free(r); return NULL;
             }
        }
    } else {
        //shift bits across limb boundaries
        uint32_t carry = 0;
        size_t i_res = limb_shift; 

        for (size_t i = 0; i < n->len; ++i, ++i_res) {
            uint64_t current_val = n->limbs[i];
            // Calculate shifted value and new carry
            uint64_t shifted_val = (current_val << bit_shift) | carry;

           
             if (i_res < r->len) {
                r->limbs[i_res] = (uint32_t)shifted_val;
             } else {
                 fprintf(stderr, "Error: Overflow in bi_shift_left_bits (bit shift loop).\n");
                 bi_free(r); return NULL;
             }
            
            carry = (uint32_t)(current_val >> (32 - bit_shift));
        }
        // Handle the final carry if any
        if (carry > 0) {
             if (i_res < r->len) {
                r->limbs[i_res] = carry;
             } else {
                  fprintf(stderr, "Error: Overflow in bi_shift_left_bits (final carry).\n");
                  bi_free(r); return NULL;
             }
        }
    }

    bi_trim(r);
    return r;
}


void bi_mod(const BigInt *a, const BigInt *m, BigInt **res)
{
    BigInt *q_discard = NULL;
    bi_divmod(a, m, &q_discard, res);
    bi_free(q_discard); 
    
}

void bi_modexp(const BigInt *base, const BigInt *exp,
               const BigInt *mod,  BigInt **res)
{
    BigInt *x = bi_copy(base);               /* moving base           */
    BigInt *y = bi_from_u64(1);              /* accumulator (result)  */
    BigInt *tmp_mul = NULL;                  /* intermediate storage for mul */
    BigInt *tmp_mod = NULL;                  /* intermediate storage for mod */
    BigInt *one = NULL;                      /* For modulus check     */

    // Basic validity checks
    if (!x || !y) {
        fprintf(stderr, "Error: Allocation failed at start of bi_modexp.\n");
        bi_free(x); bi_free(y); *res = NULL; return;
    }
    // Modulus must be >= 2 for typical modular exponentiation
    one = bi_from_u64(1);
    if (!one) { goto modexp_error;} 
    if (bi_cmp(mod, one) <= 0) {
         fprintf(stderr, "Error: Modulus must be >= 2 for bi_modexp.\n");
         bi_free(one); one = NULL; goto modexp_error;
    }
    bi_free(one); one = NULL;

    
    bi_mod(x, mod, &tmp_mod);
    if (!tmp_mod) { goto modexp_error; } 
    bi_free(x); x = tmp_mod; tmp_mod = NULL;

    size_t bits = bi_bitlen(exp);
    // Handle exp = 0 case (result should be 1)
    if (bits == 1 && exp->limbs[0] == 0) {
        bi_free(x); 
        bi_free(y); 
        *res = bi_from_u64(1); 
        return;
    }


    for (size_t i = 0; i < bits; ++i) {
        // If i-th bit of exp is 1, y = (y * x) mod mod
        if ((exp->limbs[i/32] >> (i%32)) & 1) {
            bi_mul(y, x, &tmp_mul);    
            if (!tmp_mul) {goto modexp_error; }

            bi_mod(tmp_mul, mod, &tmp_mod); 
            if (!tmp_mod) {goto modexp_error; }

            bi_free(tmp_mul); tmp_mul = NULL;
            bi_free(y);                      
            y = tmp_mod; tmp_mod = NULL;     
        }

        // Square step: x = (x * x) mod mod

        if (i < bits - 1) {
            bi_mul(x, x, &tmp_mul);       
            if (!tmp_mul) {goto modexp_error; }

            bi_mod(tmp_mul, mod, &tmp_mod); 
            if (!tmp_mod) {goto modexp_error; }

            bi_free(tmp_mul); tmp_mul = NULL;
            bi_free(x);                      
            x = tmp_mod; tmp_mod = NULL;     
        }
    }

    bi_free(x); 
    *res = y;   
    return;

modexp_error:
    
    fprintf(stderr, "Error during bi_modexp calculation.\n");
    bi_free(x);
    bi_free(y);
    bi_free(tmp_mul);
    bi_free(tmp_mod);
    bi_free(one); 
    *res = NULL; 
}


void bi_gcd(const BigInt *a, const BigInt *b, BigInt **res)
{
    BigInt *x = bi_copy(a);
    BigInt *y = bi_copy(b);
    BigInt *zero = bi_from_u64(0);
    BigInt *tmp;

    if (!x || !y || !zero) { 
        bi_free(x); bi_free(y); bi_free(zero);
        *res = NULL; return;
    }

    while (bi_cmp(y, zero) != 0) { // while y != 0
        bi_mod(x, y, &tmp); // tmp = x mod y
        if (!tmp) { 
             bi_free(x); bi_free(y); bi_free(zero);
             *res = NULL; return;
        }
        bi_free(x); 
        x = y;      
        y = tmp;    
    }

    bi_free(y); 
    bi_free(zero);
    *res = x;   
}


bool bi_modinv(const BigInt *a, const BigInt *m, BigInt **inv)
{
    // Ensure m > 1
    BigInt *one_check = bi_from_u64(1);
    if (!one_check) { *inv = NULL; return false; }
    if (bi_cmp(m, one_check) <= 0) {
        fprintf(stderr, "Error: Modulus must be > 1 for bi_modinv.\n");
        bi_free(one_check);
        *inv = NULL; return false;
    }
    bi_free(one_check); one_check = NULL; 

    // Reduce a mod m initially
    BigInt *a_reduced = NULL;
    bi_mod(a, m, &a_reduced);
    if (!a_reduced) { *inv = NULL; return false; } 

    // If a mod m is 0, inverse doesn't exist
    BigInt *zero_check = bi_from_u64(0);
    if (!zero_check) { bi_free(a_reduced); *inv = NULL; return false; }
    if (bi_cmp(a_reduced, zero_check) == 0) {
        fprintf(stderr, "Error: Cannot compute inverse of 0 mod m.\n");
        bi_free(a_reduced); bi_free(zero_check);
        *inv = NULL; return false;
    }
    bi_free(zero_check); zero_check = NULL; 


    // Standard Extended Euclidean Algorithm variables
    BigInt *t_prev = NULL, *t_curr = NULL; 
    BigInt *s_prev = NULL, *s_curr = NULL; // Coefficients for 'a'
    BigInt *q = NULL, *tmp_r = NULL, *term = NULL, *tmp_s = NULL, *sub_res = NULL;

    // Initialize based on standard algorithm for inverse of 'a' mod 'm'
    t_prev = bi_copy(m);             // t_prev = m
    t_curr = a_reduced;              // t_curr = a mod m
    s_prev = bi_from_u64(0);         // s_prev = 0
    s_curr = bi_from_u64(1);         // s_curr = 1
    BigInt *zero = bi_from_u64(0);   // Re-initialize zero locally
    BigInt *one = bi_from_u64(1);    // Re-initialize one locally


    if (!t_prev || !t_curr || !s_prev || !s_curr || !zero || !one) goto modinv_error_cleanup_std;

     while (bi_cmp(t_curr, zero) != 0) { // Loop while current remainder != 0
        // Calculate quotient q and remainder r (new t_curr)
        bi_divmod(t_prev, t_curr, &q, &tmp_r); // tmp_r = t_prev % t_curr
        if (!q || !tmp_r) goto modinv_error_cleanup_std;

        // Update t: t_prev = t_curr, t_curr = tmp_r
        bi_free(t_prev);
        t_prev = t_curr;
        t_curr = tmp_r; tmp_r = NULL;

        // Update s: new_s = s_prev - q * s_curr (mod m)
        // term = (q * s_curr) mod m
        bi_mul(q, s_curr, &term);
        if (!term) goto modinv_error_cleanup_std;
        bi_mod(term, m, &tmp_s);  // tmp_s = term mod m
        if (!tmp_s) goto modinv_error_cleanup_std;
        bi_free(term); term = tmp_s; tmp_s = NULL; // term = (q*s_curr) mod m

        // Calculate new_s = (s_prev - term + m) mod m
        if (bi_cmp(s_prev, term) >= 0) {
            bi_sub(s_prev, term, &sub_res); 
            if (!sub_res) goto modinv_error_cleanup_std;
        } else {
            // Calculate m - (term - s_prev)
            bi_sub(term, s_prev, &tmp_s); 
            if (!tmp_s) goto modinv_error_cleanup_std;
             assert(bi_cmp(m, tmp_s) >= 0 && "m < (term-s_prev) in modinv");
             bi_sub(m, tmp_s, &sub_res); 
             if (!sub_res) {bi_free(tmp_s); goto modinv_error_cleanup_std;}
             bi_free(tmp_s); tmp_s = NULL;
        }
        
        bi_free(s_prev);
        s_prev = s_curr;
        s_curr = sub_res; sub_res = NULL; 

        bi_free(q); q = NULL;
        bi_free(term); term = NULL;
     } 

     
     if (bi_cmp(t_prev, one) != 0) {
        fprintf(stderr, "Error: Inverse does not exist (gcd is not 1).\n");
        bi_free(t_prev); 
        t_prev = NULL;
        *inv = NULL;
     } else {
        bi_mod(s_prev, m, inv);
        if (!*inv) goto modinv_error_cleanup_std; 
     }


modinv_success_cleanup_std: 
    bi_free(t_prev); bi_free(t_curr); bi_free(s_prev); bi_free(s_curr);
    bi_free(zero); bi_free(one); bi_free(q); bi_free(tmp_r);
    bi_free(term); bi_free(tmp_s); bi_free(sub_res);

    return (*inv != NULL);

modinv_error_cleanup_std: 
     fprintf(stderr, "Error during bi_modinv calculation.\n");
    
    bi_free(t_prev); bi_free(t_curr); bi_free(s_prev); bi_free(s_curr);
    bi_free(zero); bi_free(one); bi_free(q); bi_free(tmp_r);
    bi_free(term); bi_free(tmp_s); bi_free(sub_res);
    bi_free(a_reduced); 
    *inv = NULL; 
    return false;
}


static void print_limb(uint32_t l) { printf("%08x",l); }
void bi_print_hex(const BigInt *n)
{
    if (!n) { printf("(null)"); return; }
    // Handle case where n is zero
    if (n->len == 1 && n->limbs[0] == 0) {
         printf("0"); return;
    }

    // Find the actual most significant non-zero limb for printing
    size_t top_idx = n->len - 1;
    while (top_idx > 0 && n->limbs[top_idx] == 0) {
        top_idx--;
    }

    // Print most significant limb first, without leading zeros unless it's the only digit
    printf("%x", n->limbs[top_idx]);
    // Print remaining limbs (down to index 0) with leading zeros
    if (top_idx > 0) {
        for (size_t i = top_idx; i-- > 0;) {
            print_limb(n->limbs[i]);
        }
    }
}


bool bi_write_hex(FILE *fp,const BigInt *n)
{
    if (!fp || !n) return false;
     // Handle case where n is zero
    if (n->len == 1 && n->limbs[0] == 0) {
         fprintf(fp, "0");
         return !ferror(fp) && fprintf(fp,"\n") > 0;
    }

    // Find the actual most significant non-zero limb for printing
    size_t top_idx = n->len - 1;
    while (top_idx > 0 && n->limbs[top_idx] == 0) {
        top_idx--;
    }

    // Print most significant limb first
    fprintf(fp, "%x", n->limbs[top_idx]);
    // Print remaining limbs (down to index 0) with leading zeros
     if (top_idx > 0) {
        for (size_t i = top_idx; i-- > 0;) {
            fprintf(fp, "%08x", n->limbs[i]);
        }
     }
    return !ferror(fp) && fprintf(fp,"\n") > 0;
}

BigInt *bi_read_hex(FILE *fp)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;

    
    len = getline(&line, &cap, fp);
    if (len <= 0) {
        free(line); 
        return NULL; 
    }

    // Strip trailing newline/carriage return
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = 0; 
    }


    if (len == 0) {
        free(line);
        return bi_from_u64(0); 
    }

    
    for (ssize_t i = 0; i < len; ++i) {
        if (!isxdigit((unsigned char)line[i])) {
            fprintf(stderr, "Error: Invalid non-hex character '%c' found in input line.\n", line[i]);
            free(line);
            return NULL;
        }
    }


    // Calculate number of limbs needed (1 hex char = 4 bits)
    size_t total_bits = len * 4;
    size_t limbs = (total_bits + 31) / 32;
    if (limbs == 0) limbs = 1; // Minimum one limb for zero or small numbers

    BigInt *n = bi_new(limbs);
    if (!n) { free(line); return NULL; }

    // Parse hex string into limbs (little-endian storage)
    size_t current_hex_pos = len; // Start from the end of the hex string
    for (size_t i = 0; i < n->len; ++i) { // Iterate through limbs (LSW first)
        // Determine start and length of hex chunk for this limb
        size_t chunk_len = (current_hex_pos >= 8) ? 8 : current_hex_pos;
        size_t start_pos = (current_hex_pos >= 8) ? current_hex_pos - 8 : 0;

        if (chunk_len == 0) break; 

        // Extract hex chunk into a temporary buffer for strtoul
        char buf[9] = {0}; // 8 hex chars + null terminator
        memcpy(buf, line + start_pos, chunk_len);

        
        char *endptr;
        unsigned long long limb_val_ull = strtoull(buf, &endptr, 16);

        
        if (*endptr != '\0') {
            fprintf(stderr, "Error: Invalid hex format in input line '%s'.\n", line);
            bi_free(n);
            free(line);
            return NULL;
        }
       
        if (limb_val_ull > 0xFFFFFFFFULL) {
             fprintf(stderr, "Error: Hex chunk '%s' exceeds 32 bits.\n", buf);
             bi_free(n); free(line); return NULL;
        }


        n->limbs[i] = (uint32_t)limb_val_ull;

        // Move position in hex string
        if (current_hex_pos < chunk_len) { 
             break;
        }
        current_hex_pos -= chunk_len;
    }

    free(line); 
    bi_trim(n); 
    return n;
}



#include "rsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include <ctype.h>   


static BigInt *bytes_to_bigint(const unsigned char *buf, size_t len)
{
    size_t limbs = (len > 0) ? (len + 3) / 4 : 1;
    BigInt *x = bi_new(limbs);
    if (!x) return NULL;

    if (len == 0) {
        return x;
    }

    for (size_t i = 0; i < len; ++i) {
        size_t L     = (len - 1 - i) / 4;
        size_t shift = ((len - 1 - i) % 4) * 8;
         if (L < x->len) {
            x->limbs[L] |= (uint32_t)buf[i] << shift;
         } else {
              fprintf(stderr, "Internal error: Limb index out of bounds in bytes_to_bigint.\n");
              bi_free(x);
              return NULL;
         }
    }
    bi_trim(x);
    return x;
}

static void bigint_to_bytes(const BigInt *n,
                            unsigned char **buf, size_t *len)
{
    *len = (bi_bitlen(n) + 7) / 8;
     if (*len == 0 && n->len == 1 && n->limbs[0] == 0) {
         *len = 1;
     }

    *buf = calloc(*len, 1);
    if (!*buf) {
        perror("Failed to allocate memory in bigint_to_bytes");
        *len = 0;
        return;
    }

     if (n->len == 1 && n->limbs[0] == 0 && *len == 1) {
         return;
     }

    for (size_t i = 0; i < *len; ++i) {
        size_t byte_idx_rev = i;
        size_t L = byte_idx_rev / 4;
        size_t shift = (byte_idx_rev % 4) * 8;

        if (L < n->len) {
             (*buf)[*len - 1 - byte_idx_rev] = (n->limbs[L] >> shift) & 0xFF;
        }
    }
}


static int encrypt_file(const char *in_path, const char *out_path);
static int decrypt_file(const char *in_path, const char *out_path);


int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <mode> <input_file> <output_file>\n", argv[0]);
        fprintf(stderr, "  mode: 'enc' (encrypt) or 'dec' (decrypt)\n");
        return 1;
    }

    const char *mode     = argv[1];
    const char *in_path  = argv[2];
    const char *out_path = argv[3];

    if (strcmp(mode, "enc") == 0) {
        return encrypt_file(in_path, out_path);
    } else if (strcmp(mode, "dec") == 0) {
        return decrypt_file(in_path, out_path);
    } else {
        fprintf(stderr, "Error: Invalid mode '%s'. Use 'enc' or 'dec'.\n", mode);
        return 1;
    }
}


static int encrypt_file(const char *in_path, const char *out_path)
{
    //printf("Mode: Encrypt\nInput: %s\nOutput: %s\n", in_path, out_path);

    FILE *fp = fopen(in_path, "rb");
    if (!fp) { perror(in_path); return 1; }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    if (sz < 0) { perror("ftell"); fclose(fp); return 1; }
    rewind(fp);

    unsigned char *plain = NULL;
    if (sz > 0) {
        plain = malloc(sz);
        if (!plain) { perror("malloc plain"); fclose(fp); return 1;}
        if (fread(plain, 1, sz, fp) != (size_t)sz) {
             fprintf(stderr, "Error reading input file\n");
             free(plain); fclose(fp); return 1;
        }
    } else {
         sz = 0;
    }
    fclose(fp);

    RSAKey pub = {0}, priv = {0};
    rsa_generate_keypair(&pub, &priv, 64); // 64 is unused now
    if (!rsa_save_key("public.key",  &pub,  "PUBLIC")) {
        fprintf(stderr, "Error saving public key.\n");
        if (plain) free(plain);
        rsa_free_key(&pub); rsa_free_key(&priv); return 1;
    }
    if (!rsa_save_key("private.key", &priv, "PRIVATE")) {
        fprintf(stderr, "Error saving private key.\n");
         if (plain) free(plain);
         rsa_free_key(&pub); rsa_free_key(&priv); return 1;
    }

    if (!pub.n) {
        fprintf(stderr, "Error: Public key modulus not generated.\n");
         if (plain) free(plain);
         rsa_free_key(&pub); rsa_free_key(&priv); return 1;
    }
     size_t n_bitlen = bi_bitlen(pub.n);
     if (n_bitlen <= 8) {
         fprintf(stderr, "Error: Key modulus n is too small (%zu bits).\n", n_bitlen);
         if (plain) free(plain);
         rsa_free_key(&pub); rsa_free_key(&priv); return 1;
     }
    size_t block_size = (n_bitlen - 1) / 8;

    FILE *fc = fopen(out_path, "w");
    if (!fc) {
        perror(out_path);
        if (plain) free(plain);
        rsa_free_key(&pub); rsa_free_key(&priv); return 1;
    }

    for (size_t pos = 0; pos < (size_t)sz; pos += block_size) {
        size_t chunk_len = (pos + block_size <= (size_t)sz)
                           ? block_size
                           : (size_t)sz - pos;

        if (chunk_len == 0) continue;

        BigInt *m = bytes_to_bigint(plain + pos, chunk_len);
        if (!m) { fprintf(stderr, "Error converting bytes to BigInt for block at pos %zu.\n", pos); continue; }

        BigInt *c = NULL;
        rsa_encrypt(m, &pub, &c);
        if (!c) { fprintf(stderr, "Error during RSA encryption for block at pos %zu.\n", pos); bi_free(m); continue; }

        /* store "length  HEXCIPHERTEXT" per line */
        fprintf(fc, "%zu ", chunk_len);
        if (!bi_write_hex(fc, c)) {
             fprintf(stderr, "Error writing ciphertext to file.\n");
             bi_free(m); bi_free(c); fclose(fc);
             if (plain) free(plain);
             rsa_free_key(&pub); rsa_free_key(&priv); return 1;
        }

        bi_free(m);
        bi_free(c);
    }
    fclose(fc);


    if (plain) free(plain);
    rsa_free_key(&pub);
    rsa_free_key(&priv);

    //puts("OK - Encryption complete!"); 
    return 0;
}


static int decrypt_file(const char *in_path, const char *out_path)
{

    RSAKey priv = {0};
    if (!rsa_load_key("private.key", &priv)) {
        fprintf(stderr, "Error loading private key from 'private.key'.\n");
        fprintf(stderr, "Ensure 'private.key' exists (generate via 'enc' mode first).\n");
        return 1;
    }
     if (!priv.n) {
        fprintf(stderr, "Error: Private key missing modulus n.\n");
        rsa_free_key(&priv); return 1;
     }
    size_t n_bitlen = bi_bitlen(priv.n);
     if (n_bitlen <= 8) {
         fprintf(stderr, "Error: Key modulus n is too small (%zu bits).\n", n_bitlen);
         rsa_free_key(&priv); return 1;
     }
    size_t max_block_size = (n_bitlen - 1) / 8;


    FILE *fc = fopen(in_path, "r");
    if (!fc) { perror(in_path); rsa_free_key(&priv); return 1; }

    // Dynamic buffer for recovered plaintext
    size_t recovered_capacity = 1024;
    unsigned char *recovered = malloc(recovered_capacity);
    if (!recovered) { perror("malloc recovered"); fclose(fc); rsa_free_key(&priv); return 1; }
    size_t written = 0;

    size_t block_num = 0;
    while (true) { // Loop until break
        block_num++;
        size_t chunk_len;
        int scan_result = fscanf(fc, "%zu ", &chunk_len);

        if (scan_result == EOF) { break; }
        if (scan_result != 1) {
             int c = fgetc(fc);
             while (isspace(c)) c = fgetc(fc);
             if (c == EOF) break;
            fprintf(stderr, "Error reading chunk length from ciphertext file (block %zu).\n", block_num);
            free(recovered); fclose(fc); rsa_free_key(&priv); return 1;
        }
        if (chunk_len == 0 || chunk_len > max_block_size ) {
             fprintf(stderr, "Warning: Suspicious chunk length %zu read from ciphertext file (block %zu, max expected %zu).\n",
                     chunk_len, block_num, max_block_size);
             char *line = NULL; size_t cap = 0; getline(&line, &cap, fc); free(line);
             continue;
        }

        BigInt *c = bi_read_hex(fc);
        if (!c) {
             if (feof(fc)) {
                  fprintf(stderr, "Warning: Incomplete last line in ciphertext file (block %zu).\n", block_num);
                  break;
             }
             fprintf(stderr, "Error reading ciphertext hex from file (block %zu).\n", block_num);
             free(recovered); fclose(fc); rsa_free_key(&priv); return 1;
        }

        BigInt *m = NULL;
        rsa_decrypt(c, &priv, &m);
         if (!m) {
              fprintf(stderr, "Error during RSA decryption (block %zu).\n", block_num);
              bi_free(c); free(recovered); fclose(fc); rsa_free_key(&priv); return 1;
         }

        unsigned char *chunk = NULL;
        size_t clen = 0;
        bigint_to_bytes(m, &chunk, &clen);
        if (!chunk && clen > 0) {
             fprintf(stderr, "Error converting decrypted BigInt to bytes (block %zu).\n", block_num);
             bi_free(m); bi_free(c); free(recovered); fclose(fc); rsa_free_key(&priv); return 1;
         }
         if (!chunk && clen == 0 && !(m->len == 1 && m->limbs[0] == 0)) {
              fprintf(stderr, "Internal Error: bigint_to_bytes inconsistency (block %zu).\n", block_num);
              bi_free(m); bi_free(c); free(recovered); fclose(fc); rsa_free_key(&priv); return 1;
         }

        size_t pad = 0;
        if (chunk_len > clen) {
            pad = chunk_len - clen;
        }

        if (written + chunk_len > recovered_capacity) {
             size_t new_capacity = recovered_capacity * 2;
             if (new_capacity < written + chunk_len) {
                 new_capacity = written + chunk_len + 1024;
             }
             unsigned char *new_recovered = realloc(recovered, new_capacity);
             if (!new_recovered) {
                 perror("realloc recovered buffer");
                 free(recovered); free(chunk); bi_free(m); bi_free(c); fclose(fc); rsa_free_key(&priv); return 1;
             }
             recovered = new_recovered;
             recovered_capacity = new_capacity;
        }

        if (pad > 0) {
            memset(recovered + written, 0, pad);
        }

        size_t bytes_to_copy = clen;
        if (clen > chunk_len) {
             fprintf(stderr, "Warning: Decrypted data length %zu > original chunk length %zu (block %zu). Truncating.\n",
                     clen, chunk_len, block_num);
             bytes_to_copy = chunk_len;
             pad = 0;
        }

        if (bytes_to_copy > 0) {
            if (chunk) {
                memcpy(recovered + written + pad, chunk, bytes_to_copy);
            } else {
                 fprintf(stderr, "Internal error during data copy in decryption (block %zu).\n", block_num);
                 free(recovered); free(chunk); bi_free(m); bi_free(c); fclose(fc); rsa_free_key(&priv); return 1;
            }
        }

        written += chunk_len;

        free(chunk);
        bi_free(m);
        bi_free(c);
    } 

    fclose(fc);


    FILE *fo = fopen(out_path, "wb");
    if (!fo) { perror(out_path); free(recovered); rsa_free_key(&priv); return 1; }

    if (recovered && written > 0) {
        if (fwrite(recovered, 1, written, fo) != written) {
             fprintf(stderr, "Error writing recovered plaintext to output file.\n");
             fclose(fo); free(recovered); rsa_free_key(&priv); return 1;
        }
    }

    fclose(fo);


    free(recovered);
    rsa_free_key(&priv);

    return 0;
}

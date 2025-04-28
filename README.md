# RSA C Implementation - Homework Assignment

## Introduction to RSA

RSA is a public-key cryptosystem. Its security relies on the difficulty of factoring large composite numbers. It uses a pair of keys: a public key for encryption and a private key for decryption.

The core of RSA involves modular arithmetic with very large integers. Standard C data types like `int` or `long long` are not large enough to hold the numbers required for secure RSA. This project provides a custom `BigInt` library to handle these arbitrary-precision integers.

You will notice the `BigInt` structure uses an array of `uint32_t`. A `uint32_t` is an **unsigned integer** guaranteed to be exactly **32 bits** wide. In the context of the `BigInt` library, these `uint32_t` values are treated as the "digits" or "limbs" of the very large number, stored in little-endian order. This allows the library to represent and perform arithmetic on numbers far larger than a single `uint64_t` can hold, by treating the large number as a sequence of these 32-bit chunks.

## Assignment Task: Implementing RSA Key Pair Generation

Your task is to complete or modify the `rsa_generate_keypair` function in `rsa.c`. This function is responsible for generating the public and private keys that are essential for the RSA algorithm.

For this assignment, the prime numbers $p$ and $q$, and the public exponent $e$, are **provided as hardcoded values** in `rsa.c` (`P_VAL`, `Q_VAL`, and `E_VAL`). Generating truly random, cryptographically secure large prime numbers is a complex task involving primality testing and is beyond the scope of this particular assignment. Therefore, you will use these fixed values to implement the subsequent steps of key generation.

The process of generating an RSA key pair using the provided values involves the following steps. Your task is to translate these steps into C code using the available `BigInt` library functions:

1.  **Start with the given primes $p$ and $q$, and public exponent $e$.**
    * These values are available as `uint64_t` constants `P_VAL`, `Q_VAL`, and `E_VAL` in `rsa.c`.
    * You will need to represent these standard integer values as the custom `BigInt` structures to perform calculations using the `BigInt` library functions.

2.  **Calculate the modulus, $n$.**
    * **Mathematical Step:** $n = p \times q$.
    * **Why:** The modulus $n$ is the foundation of both the public and private keys. All encryption and decryption operations in RSA are performed modulo $n$. This value, along with the public exponent $e$, forms the public key. It is also part of the private key.
    * **Implementation Focus:** You will need to perform a multiplication operation with large integers ($p$ and $q$ represented as `BigInt`s) to find $n$.

3.  **Calculate Euler's totient function of $n$, denoted as $\phi(n)$.**
    * **Mathematical Step:** $\phi(n) = (p-1) \times (q-1)$ for distinct primes $p$ and $q$.
    * **Why:** The totient value $\phi(n)$ is essential for calculating the private exponent $d$. The mathematical relationship between $e$ and $d$ is defined modulo $\phi(n)$. This $\phi(n)$ value is a temporary value needed only during key generation.
    * **Implementation Focus:** You will need to perform subtraction operations (subtracting 1 from $p$ and 1 from $q$) and then a multiplication operation to find $\phi(n)$.

4.  **Calculate the private exponent, $d$.**
    * **Mathematical Step:** $d \equiv e^{-1} \pmod{\phi(n)}$. This means $d$ is the unique integer between 1 and $\phi(n)$ such that when $d$ is multiplied by $e$, the result has a remainder of 1 when divided by $\phi(n)$.
    * **Why:** The private exponent $d$ is the secret key used for decryption. The mathematical properties of modular arithmetic ensure that decrypting a ciphertext $c$ using the private key $(n, d)$ recovers the original plaintext message $m$, i.e., $m \equiv c^d \pmod{n}$.
    * **Implementation Focus:** You will need to perform a modular multiplicative inverse operation with $e$ as the base and $\phi(n)$ as the modulus to find $d$.

5.  **Populate the `RSAKey` structures.**
    * The public key structure (`pub`) needs to store the modulus $n$ and the public exponent $e$.
    * The private key structure (`priv`) needs to store the modulus $n$ and the private exponent $d$. Make sure to handle the modulus $n$ correctly for both structures; they should each have their own copy if necessary.

6.  **Clean up allocated memory.**
    * The `BigInt` functions often allocate memory dynamically. You are responsible for freeing any `BigInt` structures that you create during the calculation steps but do not end up storing in the final `pub` or `priv` `RSAKey` structures. Proper memory management is crucial in C.

By following these steps and correctly applying the available `BigInt` functions, you will implement the RSA key pair generation process using the provided parameters.

## Relevant Files and Functions

You will primarily be working in the following files:

* `main.c`: Contains the main function, command-line argument parsing, and the high-level encryption/decryption file handling logic. It uses the `rsa.h` and `BigInt.h` functions.
* `rsa.c`: Implements the RSA algorithm functions, including key pair generation, encryption, and decryption. It relies heavily on the BigInt library for arithmetic operations. **Contains the hardcoded values for p, q, and e.**
* `rsa.h`: Header file for `rsa.c`, defining the `RSAKey` structure and function prototypes for RSA operations.
* `BigInt.c`: Implements the custom arbitrary-precision integer arithmetic library.
* `BigInt.h`: Contains the declarations for the `BigInt` structure and its functions.

To complete the key generation task, you will primarily need to understand and utilize the following functions from the `BigInt` library. You should consult the `BigInt.h` header file for their exact signatures and the `BigInt.c` source file to understand their behavior if needed:

* `BigInt *bi_from_u64(uint64_t v)`
* `BigInt *bi_copy(const BigInt *src)`
* `void bi_mul(const BigInt *a, const BigInt *b, BigInt **res)`
* `void bi_sub(const BigInt *a, const BigInt *b, BigInt **res)`
* `bool bi_modinv(const BigInt *a, const BigInt *m, BigInt **inv)`
* `void bi_free(BigInt *n)`

Your task is to determine how to use these functions in sequence within `rsa_generate_keypair` to perform the calculations described in the steps above.

## Your Task Steps:

1.  Open `rsa.c` and locate the `rsa_generate_keypair` function.
2.  Implement the key generation logic step-by-step as detailed above, using the `BigInt` functions and the hardcoded `P_VAL`, `Q_VAL`, and `E_VAL`.
3.  Ensure proper memory management by freeing temporary `BigInt`s.
4.  Assign the final calculated `BigInt`s to the appropriate fields (`n` and `exp`) in the public (`pub`) and private (`priv`) key structures.
5.  You can use the existing key saving functions (`rsa_save_key`) in `main.c` to verify that your generated keys are being saved correctly.

By completing this task, you will gain practical experience with the core calculations involved in setting up an RSA system and working with arbitrary-precision integers in C, using provided parameters.

## How to Build and Use (After Implementing Key Generation)

Follow the standard build instructions:

```bash
make
```

This will compile the source files and create an executable named `rsa_run`.

Then, you can use the `rsa_run` executable with your implemented key generation:

* **Encryption:** To generate keys and encrypt a file:
    ```bash
    ./rsa_run enc input.txt cipher.txt
    ```
    This will use your `rsa_generate_keypair` function to create `public.key` and `private.key` (based on the hardcoded values) and then encrypt `input.txt` into `cipher.txt`.

* **Decryption:** To decrypt a file using the generated private key:
    ```bash
    ./rsa_run dec cipher.txt recovered_input.txt
    ```
    This requires the `private.key` file generated by the encryption step. It will use your generated private key to decrypt `cipher.txt` back into `recovered_input.txt`.

## Implementation Details

### BigInt Library

The `BigInt` library handles large integers that exceed the standard integer types in C.
* A `BigInt` is represented by a structure containing a `len` (the number of limbs used) and `limbs` (a pointer to an array of `uint32_t`, representing the digits of the large number in little-endian order).
* Functions are provided for memory management (`bi_new`, `bi_free`, `bi_copy`), conversion from `uint64_t` (`bi_from_u64`), and utility operations like trimming leading zeros (`bi_trim`), getting the bit length (`bi_bitlen`), and comparison (`bi_cmp`).
* Core arithmetic operations (`bi_add`, `bi_sub`, `bi_mul`, `bi_mod`) are implemented to work with the `BigInt` structure.
* More advanced operations crucial for RSA, such as modular exponentiation (`bi_modexp`), greatest common divisor (`bi_gcd`), and modular inverse (`bi_modinv`), are also included.
* Functions for reading and writing BigInts in hexadecimal format to and from files (`bi_read_hex`, `bi_write_hex`) are provided for key storage.

### RSA Implementation

The RSA implementation in `rsa.c` utilizes the `BigInt` library.
* Key pair generation (`rsa_generate_keypair`) is where you will implement the calculations using the hardcoded `P_VAL`, `Q_VAL`, and `E_VAL`. The existing code calculates the modulus $n = p \times q$ and the totient $\phi(n) = (p-1) \times (q-1)$. The private exponent $d$ is computed as the modular multiplicative inverse of $e$ modulo $\phi(n)$ using `bi_modinv`.
* Encryption (`rsa_encrypt`) calculates the ciphertext $c$ from the plaintext message $m$ using the public key $(n, e)$ with the formula $c = m^e \pmod{n}$. This is done using the `bi_modexp` function.
* Decryption (`rsa_decrypt`) calculates the plaintext message $m$ from the ciphertext $c$ using the private key $(n, d)$ with the formula $m = c^d \pmod{n}$. This also uses the `bi_modexp` function.
* Key saving and loading (`rsa_save_key`, `rsa_load_key`) are handled by writing and reading the modulus $n$ and the exponent (e or d) in hexadecimal format to and from files, enclosed in simple header and footer lines.
* The `main.c` file handles reading the input file in chunks, converting each chunk to a `BigInt`, encrypting/decrypting it, and writing the resulting `BigInt` (along with its original chunk length) to the output file in hexadecimal format. It also handles the conversion back from `BigInt` to bytes during decryption.

This project provides a basic, functional implementation of the RSA algorithm for educational purposes, demonstrating the core concepts of asymmetric encryption and the necessity of arbitrary-precision arithmetic for such cryptographic systems.

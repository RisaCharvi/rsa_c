# RSA C Implementation - Homework Assignment

## Introduction to RSA

RSA is a foundational public-key cryptosystem. Its security is based on the mathematical difficulty of factoring large numbers. It enables secure communication by using a pair of mathematically linked keys: a public key for encrypting messages and a private key for decrypting them.

At its heart, RSA involves performing arithmetic operations (like multiplication, subtraction, and modular exponentiation) on extremely large integers. Standard C data types (`int`, `long long`) cannot hold numbers of the size required for secure RSA. This project provides a custom **Arbitrary-Precision Integer library (`BigInt`)** to handle these large numbers.

You will see that the `BigInt` structure uses an array of `uint32_t`. A `uint32_t` is an **unsigned integer** guaranteed to be exactly **32 bits** wide. In the `BigInt` library, these `uint32_t` values are treated as the "digits" or "limbs" of a very large number. By using an array of these 32-bit limbs, the library can represent and perform calculations on numbers far larger than could fit into a single 64-bit variable, effectively treating the large number as a sequence of these chunks.

## Assignment Task: Implementing RSA Key Pair Generation

Your primary task is to complete or modify the `rsa_generate_keypair` function in `rsa.c`. This function is responsible for generating the public and private keys that are essential for the RSA algorithm to work.

For this assignment, the necessary prime numbers $p$ and $q$, and the public exponent $e$, are **provided as hardcoded constants** (`P_VAL`, `Q_VAL`, and `E_VAL`) within the `rsa.c` file. Generating truly random, cryptographically secure large prime numbers is a complex process that is beyond the scope of this particular assignment. Therefore, you will use these fixed values to implement the subsequent steps of key generation.

The process of generating an RSA key pair using the provided values involves the following mathematical steps. Your goal is to translate these steps into C code using the available `BigInt` library functions:

1.  **Start with the given primes** $p$ **and** $q$**, and public exponent** $e$**.**

    * These are your starting ingredients, provided as `uint64_t` constants in `rsa.c`. You will need to represent these standard integer values as the custom `BigInt` structures to perform calculations using the `BigInt` library functions.

2.  **Calculate the modulus,** $n$**.**

    * **Mathematical Step:** $n = p \times q$.

    * **What this means:** The modulus $n$ is a fundamental component of both the public and private keys. All the cryptographic operations (encryption and decryption) are performed modulo $n$.

    * **Your implementation:** You will need to use a `BigInt` function that performs multiplication to calculate the product of your $p$ and $q$ `BigInt`s.

3.  **Calculate Euler's totient function of** $n$**, denoted as** $\phi(n)$**.**

    * **Mathematical Step:** $\phi(n) = (p-1) \times (q-1)$ for distinct primes $p$ and $q$.

    * **What this means:** The value $\phi(n)$ represents the count of positive integers up to $n$ that are relatively prime to $n$. This value is crucial for finding the private exponent $d$. It's a temporary value needed only during the key generation process.

    * **Your implementation:** You will need to use `BigInt` functions to perform subtraction (subtracting 1 from $p$ and 1 from $q$) and then multiplication to find the product of these results.

4.  **Calculate the private exponent,** $d$**.**

    * **Mathematical Step:** $d \equiv e^{-1} \pmod{\phi(n)}$. This is read as "$d$ is congruent to the modular multiplicative inverse of $e$ modulo $\phi(n)$." It means $d$ is the unique integer (within a certain range) such that when $d$ is multiplied by $e$, the result has a remainder of 1 when divided by $\phi(n)$.

    * **What this means:** The private exponent $d$ is the secret part of your key pair, used for decryption. The mathematical properties of this relationship ensure that decryption correctly reverses the encryption process.

    * **Your implementation:** You will need to use a `BigInt` function that calculates the modular multiplicative inverse. You will provide this function with your $e$ `BigInt` and your $\phi(n)$ `BigInt` to find the resulting $d$ `BigInt`.

5.  **Populate the `RSAKey` structures.**

    * The `RSAKey` structure (defined in `rsa.h`) holds the modulus `n` and an exponent (`exp`).

    * For the public key structure (`pub`), you will assign the `BigInt` you calculated for $n$ and the `BigInt` you created for $e$.

    * For the private key structure (`priv`), you will assign the `BigInt` you calculated for $d$ and a copy of the `BigInt` for $n$.

6.  **Clean up allocated memory.**

    * As you perform calculations, the `BigInt` functions will allocate memory for the results. Any `BigInt` structures that are temporary and not assigned to the final `pub` or `priv` `RSAKey` structures must be freed to prevent memory leaks.

By implementing these steps using the `BigInt` library, you will successfully generate an RSA key pair based on the provided parameters.

## Relevant Files

You will primarily be working with the following files:

* `main.c`: Contains the main program logic, including command-line argument parsing and the high-level flow for encryption and decryption, which calls your `rsa_generate_keypair` function.

* `rsa.c`: This is where you will implement the `rsa_generate_keypair` function. It also contains the hardcoded values for $p, q,$ and $e$, and the implementations for RSA encryption and decryption (which rely on key generation being correct).

* `rsa.h`: The header file for `rsa.c`. It defines the `RSAKey` structure and declares the functions for RSA operations, including `rsa_generate_keypair`.

* `BigInt.c`: Contains the implementation details of the arbitrary-precision integer library. This is where the actual arithmetic operations on large numbers are performed.

* `BigInt.h`: The header file for `BigInt.c`. It defines the `BigInt` structure and declares all the functions available in the `BigInt` library.

## Relevant `BigInt` Functions

To complete the key generation task, you will need to utilize several functions from the `BigInt` library. You should explore the `BigInt.h` header file to understand the exact signatures and purposes of the functions available. You will need functions for:

* Converting standard integers (`uint64_t`) into `BigInt` structures.

* Creating a copy of an existing `BigInt`.

* Performing multiplication of two `BigInt`s.

* Performing subtraction of two `BigInt`s.

* Calculating the modular multiplicative inverse of two `BigInt`s.

* Freeing the memory allocated for `BigInt` structures.

Your task is to identify the specific `BigInt` functions that correspond to these operations and use them correctly to perform the calculations outlined in the key generation steps.

## Your Task Steps:

1.  Open `rsa.c` and locate the `rsa_generate_keypair` function.

2.  Implement the key generation logic step-by-step as detailed above, using the appropriate `BigInt` functions and the hardcoded `P_VAL`, `Q_VAL`, and `E_VAL`.

3.  Ensure proper memory management by freeing temporary `BigInt`s that are no longer needed.

4.  Assign the final calculated `BigInt`s for $n$ and $e$ to the public key structure (`pub`) and a copy of the `BigInt` for $n$ and the `BigInt` for $d$ to the private key structure (`priv`).

5.  You can use the existing key saving functions (`rsa_save_key`) in `main.c` to verify that your generated keys are being saved correctly to `public.key` and `private.key`.

By completing this task, you will gain practical experience with the core calculations involved in setting up an RSA system and working with arbitrary-precision integers in C, using provided parameters.

## How to Build and Run

A `Makefile` is provided to help you compile the project.

* **Compile & Build:** Open your terminal in the project directory and run:

    ```bash
    make
    ```

    This will compile the source files and create an executable named `rsa_run`.

Then, you can use the `rsa_run` executable to test your implemented key generation:

* **Encryption (Generates Keys):** To generate keys (using your `rsa_generate_keypair` function) and encrypt a file:

    ```bash
    ./rsa_run enc input.txt cipher.txt
    ```

    This command will call your key generation function, save the generated `public.key` and `private.key` files, and then encrypt `input.txt` into `cipher.txt` using the public key.

* **Decryption:** To decrypt the `cipher.txt` file using the generated private key:

    ```bash
    ./rsa_run dec cipher.txt recovered_input.txt
    ```

    This command requires the `private.key` file generated by the encryption step. It will load the private key and use it to decrypt `cipher.txt` back into `recovered_input.txt`. You should verify that `recovered_input.txt` matches the original `input.txt`.

## Implementation Details

### BigInt Library

The `BigInt` library is designed to handle integers larger than native C types.

* The `BigInt` structure contains `len` (the number of `uint32_t` limbs used) and `limbs` (a pointer to an array of `uint32_t`, storing the number's digits in little-endian order).

* Functions like `bi_new`, `bi_free`, and `bi_copy` manage the memory for `BigInt`s.

* `bi_from_u64` converts a standard 64-bit integer into a `BigInt`.

* `bi_trim` removes leading zero limbs to keep the representation canonical.

* `bi_bitlen` returns the number of significant bits in a `BigInt`.

* `bi_cmp` compares two `BigInt`s.

* Core arithmetic operations (`bi_add`, `bi_sub`, `bi_mul`, `bi_mod`, `bi_divmod`) are implemented to work with the `BigInt` structure.

* Crucial for RSA are `bi_modexp` (modular exponentiation), `bi_gcd` (greatest common divisor), and `bi_modinv` (modular multiplicative inverse).

* Functions for hex encoding/decoding (`bi_read_hex`, `bi_write_hex`) are used for key file storage.

### RSA Implementation

The `rsa.c` file implements the RSA algorithm using the `BigInt` library.

* `rsa_generate_keypair` (your task) performs the key generation calculations using the `BigInt` functions and the hardcoded $p, q, e$.

* `rsa_encrypt` uses `bi_modexp` to compute $c = m^e \pmod{n}$.

* `rsa_decrypt` uses `bi_modexp` to compute $m = c^d \pmod{n}$.

* `rsa_save_key` and `rsa_load_key` handle reading/writing keys to files using the `BigInt` hex I/O functions.

* `main.c` handles file input/output, breaking the message into chunks, converting chunks to/from `BigInt`s, and calling the RSA encrypt/decrypt functions.

This project provides a basic, functional implementation of the RSA algorithm for educational purposes, demonstrating the core concepts of asymmetric encryption and the necessity of arbitrary-precision arithmetic for such cryptographic systems.

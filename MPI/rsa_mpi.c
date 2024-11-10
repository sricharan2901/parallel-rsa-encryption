#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <gmp.h>

// Function to compute (base^exp) % mod using GMP for large numbers
void mod_exp(mpz_t result, mpz_t base, mpz_t exp, mpz_t mod) {
    mpz_powm(result, base, exp, mod);
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size > 1) {
        if (rank == 0) {
            // Generate RSA keys (public and private)
            mpz_t p, q, n, phi_n, e, d;
            mpz_inits(p, q, n, phi_n, e, d, NULL);

            // Generating prime numbers p and q
            gmp_randstate_t state;
            gmp_randinit_default(state);
            mpz_urandomb(p, state, 512);  // Random 512-bit prime number for p
            mpz_urandomb(q, state, 512);  // Random 512-bit prime number for q

            // n = p * q
            mpz_mul(n, p, q);

            // Euler's totient function: phi(n) = (p-1)(q-1)
            mpz_t p_minus_1, q_minus_1;
            mpz_inits(p_minus_1, q_minus_1, NULL);
            mpz_sub_ui(p_minus_1, p, 1);
            mpz_sub_ui(q_minus_1, q, 1);
            mpz_mul(phi_n, p_minus_1, q_minus_1);

            // Choose e such that 1 < e < phi(n) and gcd(e, phi(n)) = 1
            mpz_set_ui(e, 65537);  // Common choice for e

            // Calculate d, the modular inverse of e mod phi(n)
            mpz_invert(d, e, phi_n);

            // Send the public key (e, n) to all processes
            if (rank == 0) {
                printf("Public Key: e = ");
                gmp_printf("%Zd\n", e);
                printf("Public Key: n = ");
                gmp_printf("%Zd\n", n);
            }

            // Prepare the message from input.txt
            FILE *file = fopen("input.txt", "r");
            if (!file) {
                perror("Failed to open input file");
                return 1;
            }

            char message[1024];
            fgets(message, sizeof(message), file);  // Read the first line from input.txt
            fclose(file);

            mpz_t m, c;
            mpz_inits(m, c, NULL);
            mpz_set_str(m, message, 10); // Convert message (string) to mpz_t

            // Encryption (c = m^e mod n)
            clock_t start_time = clock();
            mod_exp(c, m, e, n);
            clock_t end_time = clock();
            double encryption_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("Encryption Time: %f seconds\n", encryption_time);

            // Decrypt the message (m = c^d mod n)
            mpz_t decrypted_message;
            mpz_init(decrypted_message);
            start_time = clock();
            mod_exp(decrypted_message, c, d, n);
            end_time = clock();
            double decryption_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("Decryption Time: %f seconds\n", decryption_time);

            // Print decrypted message
            printf("Decrypted Message: ");
            gmp_printf("%Zd\n", decrypted_message);

            // Clean up GMP variables
            mpz_clears(p, q, n, phi_n, e, d, p_minus_1, q_minus_1, m, c, decrypted_message, NULL);
        }
    } else {
        printf("MPI requires at least 2 processes.\n");
    }

    MPI_Finalize();
    return 0;
}

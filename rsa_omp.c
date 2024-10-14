#include <stdio.h>
#include <stdint.h>
#include <omp.h>

uint64_t modMultiply(uint64_t a, uint64_t b, uint64_t mod) {
    return ((__int128)a * b) % mod;
}

uint64_t modular_exponentiation(uint64_t base, uint64_t exponent, uint64_t modulus) {
    uint64_t result = 1;
    base = base % modulus;

    #pragma omp parallel
    {
        uint64_t local_result = 1;
        uint64_t local_base = base;
        uint64_t local_exponent = exponent;

        #pragma omp for schedule(static)
        for (int i = 0; i < sizeof(exponent) * 8; i++) {
            if (local_exponent & (1ULL << i)) {
                #pragma omp critical
                {
                    local_result = modMultiply(local_result, local_base, modulus);
                }
            }
            local_base = modMultiply(local_base, local_base, modulus);
        }

        #pragma omp critical
        result = modMultiply(result, local_result, modulus);
    }

    return result;
}

int main() {
    uint64_t base = 5;
    uint64_t exponent = 117;
    uint64_t modulus = 19;

    uint64_t result = modular_exponentiation(base, exponent, modulus);
    printf("OpenMP Modular Exponentiation Result: %llu\n", result);

    return 0;
}

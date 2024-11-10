#include "rsa_omp.h"
#include <omp.h>

uint64_t modular_multiply(uint64_t a, uint64_t b, uint64_t mod) {
    return ((__int128)a * b) % mod;
}

uint64_t modular_exponentiation_openmp(uint64_t base, uint64_t exponent, uint64_t modulus) {
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
                local_result = modular_multiply(local_result, local_base, modulus);
            }
            local_base = modular_multiply(local_base, local_base, modulus);
        }

        #pragma omp critical
        result = modular_multiply(result, local_result, modulus);
    }

    return result;
}

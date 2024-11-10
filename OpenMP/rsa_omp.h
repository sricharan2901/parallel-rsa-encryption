#ifndef RSA_OPENMP_H
#define RSA_OPENMP_H

#include <stdint.h>

uint64_t modular_multiply(uint64_t a, uint64_t b, uint64_t mod);
uint64_t modular_exponentiation_openmp(uint64_t base, uint64_t exponent, uint64_t modulus);

#endif

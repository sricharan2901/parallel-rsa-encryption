#include <stdio.h>
#include <stdint.h>
#include <mpi.h>

uint64_t modMultiply(uint64_t a, uint64_t b, uint64_t mod) {
    return ((__int128)a * b) % mod;
}

uint64_t modular_exponentiation(uint64_t base, uint64_t exponent, uint64_t modulus) {
    uint64_t result = 1;
    base = base % modulus;

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    uint64_t local_result = 1;
    uint64_t local_base = base;
    uint64_t local_exponent = exponent;

    for (int i = rank; i < sizeof(exponent) * 8; i += size) {
        if (local_exponent & (1ULL << i)) local_result = modMultiply(local_result, local_base, modulus);
        local_base = modMultiply(local_base, local_base, modulus);
    }

    MPI_Reduce(&local_result, &result, 1, MPI_UINT64_T, MPI_PROD, 0, MPI_COMM_WORLD);
    return result;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    uint64_t base = 5;
    uint64_t exponent = 117;
    uint64_t modulus = 19;
    uint64_t result = modular_exponentiation(base, exponent, modulus);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) printf("MPI Modular Exponentiation Result: %llu\n", result);

    MPI_Finalize();
    return 0;
}

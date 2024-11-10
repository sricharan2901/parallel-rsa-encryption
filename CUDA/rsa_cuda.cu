
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <sys/time.h>

#define MODULUS 3233 // Example modulus for RSA (should be a product of two primes)
#define PUB_EXP 17   // Example public exponent for RSA
#define PRIV_EXP 413 // Example private exponent for RSA

__device__ unsigned long long mod_exp_cuda(unsigned long long base, unsigned long long exp, unsigned long long mod) {
    unsigned long long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return result;
}

__global__ void rsa_encrypt_kernel(unsigned char *input, unsigned long long *output, int len, unsigned long long exp, unsigned long long mod) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        output[idx] = mod_exp_cuda((unsigned long long)input[idx], exp, mod);
    }
}

__global__ void rsa_decrypt_kernel(unsigned long long *input, unsigned char *output, int len, unsigned long long exp, unsigned long long mod) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < len) {
        output[idx] = (unsigned char)mod_exp_cuda(input[idx], exp, mod);
    }
}

void rsa_encrypt(unsigned char *input, unsigned long long *output, int len) {
    unsigned char *d_input;
    unsigned long long *d_output;

    cudaMalloc((void **)&d_input, len * sizeof(unsigned char));
    cudaMalloc((void **)&d_output, len * sizeof(unsigned long long));

    cudaMemcpy(d_input, input, len * sizeof(unsigned char), cudaMemcpyHostToDevice);

    int blockSize = 256;
    int gridSize = (len + blockSize - 1) / blockSize;
    rsa_encrypt_kernel<<<gridSize, blockSize>>>(d_input, d_output, len, PUB_EXP, MODULUS);

    cudaMemcpy(output, d_output, len * sizeof(unsigned long long), cudaMemcpyDeviceToHost);

    cudaFree(d_input);
    cudaFree(d_output);
}

void rsa_decrypt(unsigned long long *input, unsigned char *output, int len) {
    unsigned long long *d_input;
    unsigned char *d_output;

    cudaMalloc((void **)&d_input, len * sizeof(unsigned long long));
    cudaMalloc((void **)&d_output, len * sizeof(unsigned char));

    cudaMemcpy(d_input, input, len * sizeof(unsigned long long), cudaMemcpyHostToDevice);

    int blockSize = 256;
    int gridSize = (len + blockSize - 1) / blockSize;
    rsa_decrypt_kernel<<<gridSize, blockSize>>>(d_input, d_output, len, PRIV_EXP, MODULUS);

    cudaMemcpy(output, d_output, len * sizeof(unsigned char), cudaMemcpyDeviceToHost);

    cudaFree(d_input);
    cudaFree(d_output);
}

int main() {
    FILE *file = fopen("input.txt", "rb");
    if (!file) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *input = (unsigned char *)malloc(fileSize);
    unsigned long long *encrypted = (unsigned long long *)malloc(fileSize * sizeof(unsigned long long));
    unsigned char *decrypted = (unsigned char *)malloc(fileSize);

    fread(input, 1, fileSize, file);
    fclose(file);

    struct timeval start, end;

    // Encryption
    gettimeofday(&start, NULL);
    rsa_encrypt(input, encrypted, fileSize);
    gettimeofday(&end, NULL);
    printf("Encryption completed.\n");
    printf("Public Key: %llu\n", PUB_EXP);
    printf("Modulus: %llu\n", MODULUS);
    printf("Encryption Time: %.6f seconds\n", ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) / 1000000.0);

    // Decryption
    gettimeofday(&start, NULL);
    rsa_decrypt(encrypted, decrypted, fileSize);
    gettimeofday(&end, NULL);
    printf("Decryption completed.\n");
    printf("Private Key: %llu\n", PRIV_EXP);
    printf("Modulus: %llu\n", MODULUS);
    printf("Decryption Time: %.6f seconds\n", ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) / 1000000.0);

    // Save decrypted output to file
    file = fopen("decrypted_output.txt", "wb");
    fwrite(decrypted, 1, fileSize, file);
    fclose(file);

    free(input);
    free(encrypted);
    free(decrypted);

    return 0;
}

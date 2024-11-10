# Parallelizing RSA Encryption and Decryption

## OpenMP
For compiling the code:-

- Sender
```
gcc sender.c rsa_openmp.o -o sender `pkg-config --cflags --libs gtk+-3.0` -fopenmp
```

- Receiver
```
gcc receiver.c rsa_openmp.o -o receiver_program `pkg-config --cflags --libs gtk+-3.0` -fopenmp -lpthread
```

## MPI

## CUDA
CUDA is run on Google Colab, T4 GPU

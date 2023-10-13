#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h> // Include OpenSSL's DES header

void encrypt(long key, char *text, int len) {
    DES_cblock des_key;
    DES_key_schedule schedule;
    DES_key_sched((DES_cblock *)&key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_ENCRYPT);
    }
}

void decrypt(long key, char *text, int len) {
    DES_cblock des_key;
    DES_key_schedule schedule;
    DES_key_sched((DES_cblock *)&key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_DECRYPT);
    }
}

int main(int argc, char *argv[]) {
    int N, id;
    MPI_Init(NULL, NULL);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Load text from a file (e.g., plaintext.txt)
    char text[1024]; // Adjust the buffer size as needed
    FILE *file = fopen("plaintext.txt", "r");
    if (file == NULL) {
        perror("Failed to open the file");
        MPI_Abort(comm, 1);
    }
    int text_len = fread(text, 1, sizeof(text), file);
    fclose(file);

    long encryption_key = 42; // Replace with your desired encryption key

    // Encrypt the loaded text
    encrypt(encryption_key, text, text_len);

    if (id == 0) {
        // Print the encrypted text
        printf("Encrypted text: %s\n", text);
    }

    // Decrypt the text (use the same key)
    decrypt(encryption_key, text, text_len);

    if (id == 0) {
        // Print the decrypted text
        printf("Decrypted text: %s\n", text);
    }

    MPI_Finalize();
}

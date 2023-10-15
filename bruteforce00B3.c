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
    double start_time, end_time; // Variables para medir el tiempo de ejecución
    double serial_runtime, parallel_runtime;

    MPI_Init(NULL, NULL);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    char text[1024]; // Adjust the buffer size as needed
    char filename[256]; // Buffer for filename input
    long encryption_key;

    if (argc != 3) {
        if (id == 0) {
            printf("Usage: %s <filename> <encryption_key>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    // Use the command line arguments for filename and encryption_key
    strcpy(filename, argv[1]);
    encryption_key = atol(argv[2]);

    start_time = MPI_Wtime(); // Registro del tiempo de inicio

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open the file");
        MPI_Abort(comm, 1);
    }
    int text_len = fread(text, 1, sizeof(text), file);
    fclose(file);

    // Encrypt the loaded text
    encrypt(encryption_key, text, text_len);

    // Print the encrypted text for each node
    printf("Node %d - Encrypted text: %s\n", id, text);

    // Decrypt the text (use the same key)
    decrypt(encryption_key, text, text_len);

    // Print the decrypted text for each node
    printf("Node %d - Decrypted text: %s\n", id, text);

    // Search for the keyword "es una prueba de" in the decrypted text
    if (strstr(text, "es una prueba de") != NULL) {
        printf("Node %d - ¡Frase o palabra encontrada!\n", id);
    } else {
        printf("Node %d - Frase o palabra no encontrada en el texto\n", id);
    }

    end_time = MPI_Wtime(); // Registro del tiempo de finalización
    printf("Node %d - Tiempo de ejecución: %f segundos\n", id, end_time - start_time);

    // Sincronizar todos los nodos
    MPI_Barrier(comm);

    // El nodo 0 calcula el tiempo de ejecución serial y calcula el speedup
    if (id == 0) {
        serial_runtime = end_time - start_time;
        parallel_runtime = MPI_Wtime() - start_time;
        
        // Broadcast the serial runtime to all processes
        MPI_Bcast(&serial_runtime, 1, MPI_DOUBLE, 0, comm);
        
        // Calculate and display the speedup
        double speedup = serial_runtime / parallel_runtime;
        printf("Speedup: %f\n", speedup);
    }

    MPI_Finalize();
}

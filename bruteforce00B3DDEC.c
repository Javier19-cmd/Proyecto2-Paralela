#include <mpi.h>
#include <openssl/des.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Función para cifrar un texto usando una clave específica
void encrypt(unsigned char *key, unsigned char *text, int len)
{
    DES_key_schedule schedule;
    // Configuración de la clave para el cifrado
    DES_key_sched((DES_cblock *)&key, &schedule);

    // Cifrado del texto
    for (int i = 0; i < len; i += 8)
    {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_ENCRYPT);
    }
}

// Función para descifrar un texto usando una clave específica
void decrypt(unsigned char *key, unsigned char *text, int len)
{
    DES_key_schedule schedule;
    // Configuración de la clave para el descifrado
    DES_key_sched((DES_cblock *)&key, &schedule);

    // Descifrado del texto
    for (int i = 0; i < len; i += 8)
    {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_DECRYPT);
    }
}

// Función principal
int main(int argc, char *argv[])
{
    int rank, numprocs;
    unsigned char key[8], text[64], decrypted_text[64];
    FILE *file;
    double start_time, end_time;
    MPI_Status status;
    int flag = 0;

    // Inicialización de MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // El proceso 0 lee el texto a cifrar y lo distribuye a todos los procesos
    if (rank == 0)
    {
        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            printf("Error al abrir el archivo.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fread(text, 1, 64, file);
        fclose(file);
        for (int i = 1; i < numprocs; i++)
        {
            MPI_Send(text, 64, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
        }
    }
    else
    {
        MPI_Recv(text, 64, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    }

    // Se inicia el temporizador
    start_time = MPI_Wtime();

    // Implementación del Dynamic Key Space Partitioning
    unsigned long long start_key = 0;
    unsigned long long end_key = 0xFFFFFFFFFFFFFFUL;
    unsigned long long step = numprocs;
    bool found = false;

    for (unsigned long long i = rank; i < end_key && !found; i += step)
    {
        // Convertir 'i' a un formato de clave de 8 bytes
        for (int j = 0; j < 8; j++)
        {
            key[j] = (i >> (8 * j)) & 0xFF;
        }

        // Cifrar y descifrar el texto para comprobar si hemos encontrado la clave correcta
        memcpy(decrypted_text, text, 64);
        encrypt(key, decrypted_text, 64);
        decrypt(key, decrypted_text, 64);

        // Comprobar si el texto descifrado coincide con el texto original
        if (memcmp(text, decrypted_text, 64) == 0)
        {
            end_time = MPI_Wtime();
            found = true;
            printf("Node %d - Encrypted text: %s\n", rank, text);
            printf("Node %d - Decrypted text: %s\n", rank, decrypted_text);
            printf("Node %d - ¡Frase o palabra encontrada!\n", rank);
            printf("Node %d - Tiempo de ejecución: %f segundos\n", rank, end_time - start_time);
            // Enviar una señal a todos los procesos para que se detengan
            for (int i = 0; i < numprocs; i++)
            {
                if (i != rank)
                {
                    MPI_Send(&flag, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                }
            }
        }

        // Comprobar si otro proceso ha encontrado la clave
        MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &flag, &status);
        if (flag)
        {
            break;
        }
    }

    // Finalización de MPI
    MPI_Finalize();
    return 0;
}

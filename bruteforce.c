// bruteforce.c
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <openssl/des.h> // Incluir OpenSSL para el cifrado DES

void decrypt(long key, unsigned char *ciph, int len){
  DES_key_schedule keysched;
  DES_cblock iv;

  // Configurar la clave DES
  DES_set_key_unchecked((const_DES_cblock *)&key, &keysched);

  // Configurar el vector de inicialización
  memset(iv, 0, sizeof(DES_cblock));

  // Decifrar el mensaje
  DES_ncbc_encrypt(ciph, ciph, len, &keysched, &iv, DES_DECRYPT);
}

char search[] = " 0x74 0x61 0x72 0x64 0x65 ";
int tryKey(long key, unsigned char *ciph, int len){
  unsigned char temp[len];
  memcpy(temp, ciph, len);
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}


// Hello World!
unsigned char cipher[] = {0x46, 0x75, 0x65, 0x20, 0x75, 0x6E, 0x61, 0x20, 0x74, 0x61, 0x72, 0x64, 0x65, 0x20, 0x68, 0x65, 0x72, 0x6D, 0x6F, 0x73, 0x61, 0x20, 0x65, 0x6E, 0x20, 0x65, 0x6C, 0x20, 0x70, 0x61, 0x72, 0x71, 0x75, 0x65, 0x2E};
  
int main(int argc, char *argv[]){



  int N, id;
  long upper = (1L << 56); // Upper bound DES keys 2^56
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  int flag;
  int ciphlen = sizeof(cipher); // Usar sizeof para obtener la longitud en bytes
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id + 1) - 1;
  if (id == N - 1) {
    // Compensar residuo
    myupper = upper;
  }

  long found = 0;
  int ready = 0;

  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  for (long i = mylower; i < myupper; ++i) {
    MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
    if (ready)
      break; // Ya encontraron, salir

    if (tryKey(i, cipher, ciphlen)) {
      found = i;
      for (int node = 0; node < N; node++) {
        MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
      }
      break;
    }
  }

  if (id == 0) {
    MPI_Wait(&req, &st);
    decrypt(found, cipher, ciphlen);
    printf("%li ", found);
    for (int i = 0; i < ciphlen; i++) {
      printf("%02x ", cipher[i]); // Imprimir los bytes descifrados en hexadecimal
    }
    printf("\n");
  }

  MPI_Finalize();
}

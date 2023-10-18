#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/des.h> // Include OpenSSL's DES header

void decrypt(unsigned char key[8], char *ciph, int len) {
    DES_key_schedule schedule;
    DES_set_key((const_DES_cblock *)key, &schedule);
    
    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((const_DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
}

char search[] = " the ";
int tryKey(unsigned char key[8], char *ciph, int len) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr(temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215};
int main(int argc, char *argv[]) {
    unsigned char key[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int ciphlen = sizeof(cipher);

    for (unsigned long i = 0; i < (1UL << 56); ++i) {
        memcpy(key, &i, sizeof(i));
        if (tryKey(key, (char *)cipher, ciphlen)) {
            decrypt(key, (char *)cipher, ciphlen);
            printf("%lu %s\n", i, cipher);
            break;
        }
    }

    return 0;
}

#include "cipher.h"
#include <stdio.h>
#include <stdlib.h>
#include "sharecommon.h"
#include "tomcrypt.h"

static int init_cbc_key(const BYTE *iv, const BYTE *key, unsigned long keylen, symmetric_CBC *cbc);

int aes_cbc_encrypt(const BYTE *in,  unsigned long len, const BYTE *iv, const BYTE *key, unsigned long keylen, BYTE **out, unsigned long *outlen)
{
    printf("aes_cbc_encrypt\n");

    symmetric_CBC cbc;
    if (init_cbc_key(iv, key, keylen, &cbc) != SUCCESS) {
        return FAILURE;
    }

    *outlen = ((int)(len + 15) / 16) * 16;
    printf("len: %lu outlen: %lu\n", len, *outlen);

    *out = (BYTE*)malloc(*outlen);

    if (cbc_encrypt(in, *out, *outlen, &cbc) != CRYPT_OK) {
        printf("aes_cbc_encrypt failed\n");
        free(*out);
        *out = NULL;
        return FAILURE;
    }  

    return SUCCESS;
}

int aes_cbc_decrypt(const BYTE *in,  unsigned long len, const BYTE *iv, const BYTE *key, unsigned long keylen, BYTE **out, unsigned long *outlen)
{
    printf("aes_cbc_decrypt\n");

    symmetric_CBC cbc;
    if (init_cbc_key(iv, key, keylen, &cbc) != SUCCESS) {
        return FAILURE;
    }

    *out = (BYTE*)malloc(len);
    //cbc_setiv(iv2, l, &cbc);
    if (cbc_decrypt(in, *out, len, &cbc) != CRYPT_OK) {
        printf("aes_cbc_decrypt failed\n");
        free(*out);
        *out = NULL;
        return FAILURE;
    }

    *outlen = (unsigned long)strlen((const char*)*out);
    printf("len: %lu outlen: %lu\n", len, *outlen);

    return SUCCESS;
}


/*************************************************************************************
 Helpers
**************************************************************************************/
static int init_cbc_key(const BYTE *iv, const BYTE *key, unsigned long keylen, symmetric_CBC *cbc) 
{
    printf("init_cbc_key\n");

    int cipher_idx = find_cipher("aes");
    if (cipher_idx < 0) {
        printf("init_cbc_key requires AES");
        return FAILURE;
    }


    if (cbc_start(cipher_idx, iv, key, keylen, 0, cbc) != CRYPT_OK) {
        printf("cbc_start failed\n");
        return FAILURE;
    }

    return SUCCESS;
}



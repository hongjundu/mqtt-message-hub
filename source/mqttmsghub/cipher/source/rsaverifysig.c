#include "cipher.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <memory.h>
#include "sharecommon.h"
#include "filepathutils.h"
#include "tomcrypt.h"
#include "base64.h"

static int read_public_key(char **keyBase64, const char *keyFilePath, char *errmsg);
static int rsa_verify_signature(const char *in,  unsigned long len, 
    const char *sigbase64, unsigned long sigbase64len,
    const char *keybase64, unsigned long keybase64len, 
    int *stat, char *errmsg);


int rsa_verify_cmd_signature(const char *keystring, const char *keyfile,const char *in,  unsigned long len, 
    const char *sigbase64, unsigned long sigbase64len,
    int *stat, char *errmsg)
{
    printf("rsa_verify_cmd_signature\n");
    *errmsg = '\0';

    char *keyBase64 = NULL;

    if (keystring) {
        keyBase64 = (char*)malloc(strlen(keystring) + 1);
        strcpy(keyBase64, keystring);
    }
    /*
     Read public key from file
    */
    else if (read_public_key(&keyBase64, keyfile, errmsg) != SUCCESS) {
        return FAILURE;
    }

    printf("public key: %s\n", keyBase64);

    int rc = rsa_verify_signature(in, len, (const char*)sigbase64, sigbase64len, (const char*)keyBase64, strlen(keyBase64), stat, errmsg);

    free(keyBase64);

    return rc;
}


static int rsa_verify_signature(const char *in,  unsigned long len, 
    const char *sigbase64, unsigned long sigbase64len,
    const char *keybase64, unsigned long keybase64len, 
    int *stat, char *errmsg)
{
    printf("rsa_verify_signature\n");

    *errmsg = '\0';

    BYTE pubKeyBuffer[1024 * 5];
    unsigned long pubKeyBufferLen = 0;

    memset(pubKeyBuffer, 0, sizeof(pubKeyBuffer));
    if(base64_decrypt(keybase64, keybase64len, pubKeyBuffer, &pubKeyBufferLen) != BASE64_OK) {
        strcpy(errmsg,"base64_decrypt public key failed");
        printf("%s\n", errmsg);
        return FAILURE;
    }

    rsa_key pubKey;
    int rc = rsa_import(pubKeyBuffer, pubKeyBufferLen, &pubKey);
    if (rc != CRYPT_OK) {
        sprintf(errmsg, "rsa_import pubic key failed: %d", rc);
        printf("%s\n", errmsg);
        return FAILURE;
    }

    BYTE *sig = (BYTE*)malloc(sigbase64len);
    unsigned long siglen = 0;

    if (base64_decrypt(sigbase64, sigbase64len, sig, &siglen) != BASE64_OK) {
        strcpy(errmsg, "base64_decrypt failed");
        printf("%s\n",errmsg);

        rsa_free(&pubKey);
        return FAILURE;
    }

    int hash_idx = find_hash("sha256");
    if (hash_idx < 0)  {
        strcpy(errmsg, "find sha256 hash index failed");
        printf("%s\n", errmsg);

        free(sig);
        rsa_free(&pubKey);
        return FAILURE;
    }

    BYTE* hashResult = (BYTE*)malloc(sha256_desc.hashsize);
    hash_state md;
    sha256_init(&md);
    //Process the text - remember you can call process() multiple times
    sha256_process(&md, (const BYTE*) in, len);
    //Finish the hash calculation
    sha256_done(&md, hashResult);
    printf("sha256 result: %s\n", hashResult);

    const int padding = LTC_LTC_PKCS_1_V1_5;
    const unsigned long saltlen = 0;

    rc = rsa_verify_hash_ex(sig, siglen, hashResult, sha256_desc.hashsize, padding, hash_idx, saltlen, stat, &pubKey);

    rsa_free(&pubKey);
    free(hashResult);
    free(sig);

    if (rc != CRYPT_OK) {
        sprintf(errmsg, "Verify error %d", rc);
        printf("%s\n", errmsg);
        return FAILURE;
    }

    printf("Verify status: %s\n", *stat ? "Valid" : "Invalid");

    return SUCCESS;
}

/*
keyBase64 MUST be freed
*/
static int read_public_key(char **keyBase64, const char *keyFilePath, char *errmsg) 
{
    printf("read_public_key\n");

    *errmsg = 0;

    if (!file_exists(keyFilePath)) {
        sprintf(errmsg, "Key file %s doesn't exist", keyFilePath);
        return FAILURE;
    }

    size_t readSize = read_file_string(keyBase64,keyFilePath);

    if (readSize <= 0) {
        sprintf(errmsg, "Read size is 0: %s", keyFilePath);
        return FAILURE;
    }

    return SUCCESS;
}
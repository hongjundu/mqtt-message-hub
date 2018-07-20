#include "cipher.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include "sharecommon.h"
#include "secdef.h"

#define PAYLOAD_HEADER_SIZE sizeof(mqtt_encrypted_payload_header)
#define CBC_EX_DATA_SIZE    sizeof(encrypt_cbc_ex_data)

static BYTE cbc_key[16]= CBC_ENCRYPT_KEY;

int aes_cbc_encrypt(const BYTE *in,  unsigned long len, const BYTE *iv, const BYTE *key, unsigned long keylen, BYTE **out, unsigned long *outlen);
int aes_cbc_decrypt(const BYTE *in,  unsigned long len, const BYTE *iv, const BYTE *key, unsigned long keylen, BYTE **out, unsigned long *outlen);
static void generate_random_iv(BYTE *iv, unsigned long len);
/*
  [ENCODE_METHOD: 2 BYTES] [VERSION: 2 BYTES] [CONTENT LENGTH: 4 BYTES] [RESERVED: 4 BYTES] [CBC IV: 16 BYTES] [CONTENT...]
*/

int mqtt_payload_encrypt(const char *content, unsigned long len, BYTE **payload, unsigned long *payloadlen, mqtt_encrypted_payload_header *header)
{
    printf("mqtt_payload_encrypt\n");

    BYTE *encryptContent = NULL;
    unsigned long encryptContentLen = 0;

    encrypt_cbc_ex_data cbcExData;
    memset(&cbcExData, 0, CBC_EX_DATA_SIZE);
    generate_random_iv(cbcExData.iv, sizeof(cbcExData.iv) / sizeof(BYTE));

    /* encrypt mqtt payload content */
    int rc = aes_cbc_encrypt((BYTE*)content, len, cbcExData.iv, cbc_key, sizeof(cbc_key) / sizeof(cbc_key[0]), &encryptContent, &encryptContentLen);
    if (rc != SUCCESS) {
        printf("%s\n", "aes_cbc_encrypt failed");
        return rc;
    }

    /* payload size */
    *payloadlen = PAYLOAD_HEADER_SIZE + CBC_EX_DATA_SIZE + encryptContentLen;
    
    /* alloc memory for payload buffer */
    *payload = (BYTE*)malloc(*payloadlen);

    /* set header */
    header->encryptMethod = ENCRYPT_CBC;
    header->version = ENCRYPT_CBC_VERSION;
    header->contentSize = len;
    header->reserved = 0;

    /* add header */
    memcpy(*payload, header, PAYLOAD_HEADER_SIZE);

    /* add cbc ex data */
    memcpy(*payload + PAYLOAD_HEADER_SIZE, &cbcExData, CBC_EX_DATA_SIZE);

    /* add encrypted content */
    if (encryptContent && encryptContentLen > 0) {
        memcpy(*payload + PAYLOAD_HEADER_SIZE + CBC_EX_DATA_SIZE, encryptContent, encryptContentLen);
    }
    if (encryptContent) {
        free(encryptContent);
    }

    return SUCCESS;
}

int mqtt_payload_decrypt(const BYTE *payload,  unsigned long len, char **content, unsigned long *contentlen, mqtt_encrypted_payload_header *header)
{
    printf("mqtt_payload_decrypt\n");

    if (len <= PAYLOAD_HEADER_SIZE + CBC_EX_DATA_SIZE) {
        printf("size of payload is wrong\n");
        return FAILURE;
    }

    /* header */
    memcpy(header, payload, PAYLOAD_HEADER_SIZE);

    /* cbc ex data */
    if (header->encryptMethod != ENCRYPT_CBC) {
        printf("encrypt method is not supported\n");
        return FAILURE;
    }

    encrypt_cbc_ex_data cbcExData;
    memset(&cbcExData, 0, CBC_EX_DATA_SIZE);
    memcpy(&cbcExData, payload + PAYLOAD_HEADER_SIZE, CBC_EX_DATA_SIZE);

    int rc = aes_cbc_decrypt(payload + PAYLOAD_HEADER_SIZE + CBC_EX_DATA_SIZE, len - PAYLOAD_HEADER_SIZE - CBC_EX_DATA_SIZE, cbcExData.iv, cbc_key, sizeof(cbc_key) / sizeof(cbc_key[0]), (BYTE**)content, contentlen);
    if (rc != SUCCESS) {
        printf("aes_cbc_decrypt failed\n");
    }

    return rc;
}


/*************************************************************************************
 Helpers
**************************************************************************************/
static void generate_random_iv(BYTE *iv, unsigned long len)
{
    srand(time(0));

    int i = 0;
    for(i = 0; i < len; i++) {
        *(iv + i) = rand() % 256;
    }
}


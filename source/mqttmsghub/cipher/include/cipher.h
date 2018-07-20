#pragma once

#include "sharecommon.h"
/*
 MQTT payload encrypt methods, might be extended...
*/
enum { ENCRYPT_CBC = 1 };

/*
 MQTT payload encrypt version
*/
enum { ENCRYPT_CBC_VERSION_V1 = 1, ENCRYPT_CBC_VERSION = ENCRYPT_CBC_VERSION_V1 };

typedef struct {
    unsigned short encryptMethod;
    unsigned short version;
    unsigned int contentSize;
    unsigned int reserved;
}mqtt_encrypted_payload_header;

typedef struct {
    BYTE iv[16];
}encrypt_cbc_ex_data;
/*
  if keystring is not null, use keystring, otherwise, read key string for keyfile
*/
int rsa_verify_cmd_signature(const char *keystring, const char *keyfile,const char *in,  unsigned long len, const char *sigbase64, unsigned long sigbase64len, int *stat, char *errmsg);

int mqtt_payload_encrypt(const char *content,  unsigned long len, BYTE **payload, unsigned long *payloadlen, mqtt_encrypted_payload_header *header);

int mqtt_payload_decrypt(const BYTE *playload, unsigned long len, char **content, unsigned long *contentlen, mqtt_encrypted_payload_header *header);
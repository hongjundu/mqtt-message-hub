#ifndef __BASE64_H__
#define __BASE64_H__

enum { BASE64_OK = 0, BASE64_INVALID };

#define BASE64_ENCODE_OUT_SIZE(s)	(((s) + 2) / 3 * 4)
#define BASE64_DECODE_OUT_SIZE(s)	(((s)) / 4 * 3)

int base64_encrypt(const unsigned char *in, unsigned long inlen, char *out);

int base64_decrypt(const char *in, unsigned long inlen, unsigned char *out, unsigned long *outLen);

unsigned long calc_base64_encrypted_size(unsigned long input_size);

#endif /* __BASE64_H__ */


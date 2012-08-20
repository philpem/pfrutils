#ifndef FILMTABLE_CRYPT_H
#define FILMTABLE_CRYPT_H

void filmtable_crypto_init(void);
unsigned char filmtable_crypto_decrypt(unsigned char in);
unsigned char filmtable_crypto_encrypt(unsigned char in);

#endif // FILMTABLE_CRYPT_H

#ifndef fido2hmacsecret_cryptography_H
#define fido2hmacsecret_cryptography_H

#include "invocation.h"
#include "serialization_types.h"
#include "stdlib.h"
#include <sodium.h>

#define KEY_SIZE crypto_secretbox_KEYBYTES

typedef struct key_spec_t {
	char *passphrase;
	unsigned char *kdf_salt;
	size_t kdf_salt_size;
	unsigned long long opslimit;
	size_t memlimit;
	int algorithm;
} key_spec_t;

unsigned char *derive_key(key_spec_t *key_spec);
void free_key_spec(key_spec_t *spec);
void free_key(unsigned char *key);
key_spec_t *
make_key_spec_from_passphrase_and_cleartext(char *passphrase,
                                            deserialized_cleartext *cleartext);
key_spec_t *make_new_key_spec_from_invocation(invocation_state_t *invocation);

#endif

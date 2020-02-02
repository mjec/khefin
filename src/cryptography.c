#include "cryptography.h"

#include "exit.h"
#include "invocation.h"
#include "memory.h"
#include <sodium.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned char *derive_key(key_spec_t *key_spec) {
	if (key_spec->kdf_salt_size != crypto_pwhash_SALTBYTES) {
		err(EXIT_PROGRAMMER_ERROR,
		    "KDF salt is of wrong size (is %zu bytes, should be %d bytes)",
		    key_spec->kdf_salt_size, crypto_pwhash_SALTBYTES);
	}

	unsigned char *key_bytes =
	    malloc_or_exit(KEY_SIZE, "passphrase-derived key");

	if (crypto_pwhash(key_bytes, KEY_SIZE, key_spec->passphrase,
	                  strlen(key_spec->passphrase), key_spec->kdf_salt,
	                  key_spec->opslimit, key_spec->memlimit,
	                  key_spec->algorithm) != 0) {
		err(EXIT_OUT_OF_MEMORY,
		    "Unable to derive key from passphrase (out of memory?)");
	}

	return key_bytes;
}

void free_key(unsigned char *key) {
	if (key == NULL) {
		return;
	}
	sodium_memzero(key, KEY_SIZE);
	free(key);
}

key_spec_t *
make_key_spec_from_passphrase_and_cleartext(char *passphrase,
                                            deserialized_cleartext *cleartext) {
	key_spec_t *keyspec = malloc_or_exit(
	    sizeof(key_spec_t), "password-derived key specificications");
	keyspec->passphrase = strdup_or_exit(
	    passphrase, "passphrase in password-derived key specificications");
	keyspec->opslimit = cleartext->opslimit;
	keyspec->memlimit = cleartext->memlimit;
	keyspec->algorithm = cleartext->algorithm;
	keyspec->kdf_salt =
	    malloc_or_exit(cleartext->kdf_salt_size,
	                   "salt in password-derived key specificications");
	memcpy(keyspec->kdf_salt, cleartext->kdf_salt, cleartext->kdf_salt_size);
	keyspec->kdf_salt_size = cleartext->kdf_salt_size;

	return keyspec;
}

key_spec_t *make_new_key_spec_from_invocation(invocation_state_t *invocation) {
	key_spec_t *keyspec = malloc_or_exit(
	    sizeof(key_spec_t), "password-derived key specificications");
	keyspec->passphrase =
	    strdup_or_exit(invocation->passphrase,
	                   "passphrase in password-derived key specificications");

	switch (invocation->kdf_hardness) {
	case kdf_hardness_low:
		keyspec->opslimit = crypto_pwhash_OPSLIMIT_INTERACTIVE;
		keyspec->memlimit = crypto_pwhash_MEMLIMIT_INTERACTIVE;
		break;
	case kdf_hardness_medium:
		keyspec->opslimit = crypto_pwhash_OPSLIMIT_MODERATE;
		keyspec->memlimit = crypto_pwhash_MEMLIMIT_MODERATE;
		break;
	case kdf_hardness_high:
		keyspec->opslimit = crypto_pwhash_OPSLIMIT_SENSITIVE;
		keyspec->memlimit = crypto_pwhash_MEMLIMIT_SENSITIVE;
		break;
	case kdf_hardness_unspecified:
	case kdf_hardness_invalid:
		errx(EXIT_PROGRAMMER_ERROR,
		     "BUG (%s:%d): invalid KDF hardness passed in (%d)\n", __func__,
		     __LINE__, invocation->kdf_hardness);
		break;
	}

	if (keyspec->opslimit < 3) {
		// opslimit must be at least 3 because we're using Argon2i:
		// https://download.libsodium.org/doc/password_hashing/default_phf#notes
		keyspec->opslimit = 3;
	}

	keyspec->algorithm = crypto_pwhash_ALG_ARGON2I13;
	keyspec->kdf_salt =
	    malloc_or_exit(crypto_pwhash_SALTBYTES,
	                   "salt in password-derived key specificications");
	randombytes_buf(keyspec->kdf_salt, crypto_pwhash_SALTBYTES);
	keyspec->kdf_salt_size = crypto_pwhash_SALTBYTES;

	return keyspec;
}

void free_key_spec(key_spec_t *spec) {
	if (spec == NULL) {
		return;
	}
	if (spec->passphrase != NULL) {
		sodium_memzero(spec->passphrase, strlen(spec->passphrase));
		free(spec->passphrase);
	}
	if (spec->kdf_salt != NULL) {
		free(spec->kdf_salt);
	}
	free(spec);
}

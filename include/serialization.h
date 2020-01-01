#ifndef fido2hmacsecret_serialization_H
#define fido2hmacsecret_serialization_H

#include <cbor.h>
#include <stdint.h>

#include "authenticator.h"

#define SERIALIZATION_MAX_VERSION 1

#ifndef LARGEST_VALID_PAYLOAD_SIZE_BYTES
#define LARGEST_VALID_PAYLOAD_SIZE_BYTES 10485760
#endif

#define OBFUSCATED_DEVICE_SENTINEL 0

#ifdef DEBUG
#define FIELD_COUNTER_ASSERT_START unsigned short __field_counter_assert = 0;
#define FIELD_COUNTER_ASSERT(function, expected, file, line)                   \
	if (__field_counter_assert++ != expected)                                  \
		errx(EXIT_PROGRAMMER_ERROR,                                            \
		     "BUG (%s:%d): Serializing in %s is messed up: field %d should "   \
		     "be at "                                                          \
		     "position %d",                                                    \
		     file, line, function, __field_counter_assert, expected);
#else
#define FIELD_COUNTER_ASSERT_START
#define FIELD_COUNTER_ASSERT(function, expected, file, line)
#endif

typedef struct deserialized_cleartext {
	uint8_t version;
	unsigned char *device_aaguid;
	size_t device_aaguid_size;

	unsigned char *kdf_salt;
	size_t kdf_salt_size;

	unsigned long long opslimit;
	size_t memlimit;
	int algorithm;

	unsigned char *nonce;
	size_t nonce_size;

	unsigned char *encrypted_data;
	size_t encrypted_data_size;
} deserialized_cleartext;

typedef struct deserialized_secrets {
	uint8_t version;

	char *relying_party_id;

	unsigned char *credential_id;
	size_t credential_id_size;

	unsigned char *salt;
	size_t salt_size;

} deserialized_secrets;

authenticator_parameters_t *
build_authenticator_parameters_from_deserialized_cleartext_and_passphrase(
    deserialized_cleartext *cleartext, char *passphrase);
deserialized_cleartext *
build_deserialized_cleartext_from_authenticator_parameters_and_passphrase(
    authenticator_parameters_t *authenticator_params, char *passphrase);
void free_cleartext(deserialized_cleartext *clear);
void free_secrets(deserialized_secrets *secret);
void write_cleartext_to_file(deserialized_cleartext *cleartext,
                             const char *path);
deserialized_cleartext *load_cleartext_from_file(const char *path);
deserialized_secrets *load_secrets_from_bytes(unsigned char *decrypted,
                                              size_t decrypted_size);
const char *get_cbor_error_string(cbor_error_code code);

#endif

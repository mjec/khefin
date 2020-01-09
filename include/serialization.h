#ifndef fido2hmacsecret_serialization_H
#define fido2hmacsecret_serialization_H

#include <cbor.h>

#include "authenticator.h"
#include "cryptography.h"
#include "serialization_types.h"

#define SERIALIZATION_MAX_VERSION 1

#ifndef LARGEST_VALID_PAYLOAD_SIZE_BYTES
#define LARGEST_VALID_PAYLOAD_SIZE_BYTES 10485760
#endif

#define OBFUSCATED_DEVICE_SENTINEL 0

#ifdef DEBUG
#define FIELD_COUNTER_ASSERT_START unsigned short __field_counter_assert = 0;
#define FIELD_COUNTER_ASSERT(function, expected, line)                         \
	if (__field_counter_assert++ != expected)                                  \
		errx(EXIT_PROGRAMMER_ERROR,                                            \
		     "BUG (%s:%d): Serializing s is messed up: field %d should be at " \
		     "position %d",                                                    \
		     function, line, __field_counter_assert, expected);
#else
#define FIELD_COUNTER_ASSERT_START
#define FIELD_COUNTER_ASSERT(function, expected, line)
#endif

authenticator_parameters_t *
build_authenticator_parameters_from_deserialized_cleartext_and_key(
    deserialized_cleartext *cleartext, unsigned char *key_bytes);
deserialized_cleartext *
build_deserialized_cleartext_from_authenticator_parameters_and_key_spec(
    authenticator_parameters_t *authenticator_params, key_spec_t *key_spec);
void free_cleartext(deserialized_cleartext *clear);
void free_secrets(deserialized_secrets *secret);
void write_cleartext_to_file(deserialized_cleartext *cleartext,
                             const char *path);
deserialized_cleartext *load_cleartext_from_file(const char *path);
deserialized_secrets *load_secrets_from_bytes(unsigned char *decrypted,
                                              size_t decrypted_size);
const char *get_cbor_error_string(cbor_error_code code);

#endif

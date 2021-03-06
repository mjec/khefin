#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <cbor.h>

#include "authenticator.h"
#include "cryptography.h"
#include "serialization_types.h"

#define SERIALIZATION_MAX_VERSION 1

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
build_authenticator_parameters_from_deserialized_cleartext_and_key_and_mixin(
    deserialized_cleartext *cleartext, unsigned char *key_bytes, char *mixin);
deserialized_cleartext *
build_deserialized_cleartext_from_authenticator_parameters_and_key_spec(
    authenticator_parameters_t *authenticator_params, key_spec_t *key_spec);
void free_cleartext(deserialized_cleartext *clear);
void free_secrets(deserialized_secrets *secret);
encoded_file *write_cleartext(deserialized_cleartext *cleartext,
                              const char *path);
deserialized_cleartext *load_cleartext(encoded_file *file);
deserialized_secrets *load_secrets_from_bytes(unsigned char *decrypted,
                                              size_t decrypted_size);
const char *get_cbor_error_string(cbor_error_code code);

#endif

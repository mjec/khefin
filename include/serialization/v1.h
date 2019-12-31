#ifndef fido2hmacsecret_serialization_v1_H
#define fido2hmacsecret_serialization_v1_H

#include <cbor.h>

#include "../serialization.h"

deserialized_cleartext *
deserialize_cleartext_from_cbor_v1(cbor_item_t *cbor_root);
cbor_item_t *serialize_cleartext_to_cbor_v1(deserialized_cleartext *clear);

deserialized_secrets *deserialize_secrets_from_cbor_v1(cbor_item_t *cbor_root);
cbor_item_t *serialize_secrets_to_cbor_v1(deserialized_secrets *secrets);

#define SERIALIZATION_VERSION 1

#define CLEAR_FIELD_VERSION 0
#define CLEAR_FIELD_DEVICE_VENDOR 1
#define CLEAR_FIELD_DEVICE_PRODUCT 2
#define CLEAR_FIELD_KDF_SALT 3
#define CLEAR_FIELD_OPSLIMIT 4
#define CLEAR_FIELD_MEMLIMIT 5
#define CLEAR_FIELD_ALGORITHM 6
#define CLEAR_FIELD_NONCE 7
#define CLEAR_FIELD_ENCRYPTED_DATA 8

#define CLEAR_COUNT_OF_FIELDS 9

#define ENCRYPTED_FIELD_VERSION 0
#define ENCRYPTED_FIELD_RP_ID 1
#define ENCRYPTED_FIELD_CREDENTIAL_ID 2
#define ENCRYPTED_FIELD_HMAC_SALT 3

#define ENCRYPTED_COUNT_OF_FIELDS 4

#endif

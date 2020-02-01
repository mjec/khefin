#ifndef fido2hmacsecret_serialization_types_H
#define fido2hmacsecret_serialization_types_H

#include <stdint.h>
#include <stdlib.h>

typedef struct encoded_file {
	char *path;
	unsigned char *data;
	size_t length;
} encoded_file;

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

#endif

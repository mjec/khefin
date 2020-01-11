#include <cbor.h>
#include <sodium.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "authenticator.h"
#include "cryptography.h"
#include "exit.h"
#include "serialization.h"
#include "serialization/v1.h"

authenticator_parameters_t *
build_authenticator_parameters_from_deserialized_cleartext_and_key_and_mixin(
    deserialized_cleartext *cleartext, unsigned char *key_bytes, char *mixin) {
	unsigned char *decrypted =
	    malloc(cleartext->encrypted_data_size - crypto_secretbox_MACBYTES);
	CHECK_MALLOC(cleartext->encrypted_data, "encrypted data");
	if (crypto_secretbox_open_easy(decrypted, cleartext->encrypted_data,
	                               cleartext->encrypted_data_size,
	                               cleartext->nonce, key_bytes) != 0) {
		errx(EXIT_BAD_PASSPHRASE,
		     "Could not decrypt secrets; this likely means "
		     "the passphrase was wrong");
	}
	deserialized_secrets *secrets = load_secrets_from_bytes(
	    decrypted, cleartext->encrypted_data_size - crypto_secretbox_MACBYTES);
	free(decrypted);

	authenticator_parameters_t *params = allocate_parameters_except_rpid(
	    secrets->credential_id_size, secrets->salt_size);
	memcpy(params->credential_id, secrets->credential_id,
	       secrets->credential_id_size);
	params->relying_party_id = strdup(secrets->relying_party_id);
	CHECK_MALLOC(params->relying_party_id,
	             "relying party id in authenticator parameters");
	memcpy(params->salt, secrets->salt, secrets->salt_size);
	free_secrets(secrets);
	secrets = NULL;

	if (mixin != NULL) {
		unsigned char *hashed_mixin = malloc(params->salt_size);
		if (crypto_generichash(hashed_mixin, params->salt_size,
		                       (unsigned char *)mixin, strlen(mixin), NULL,
		                       0) != 0) {
			errx(EXIT_CRYPTOGRAPHY_ERROR, "Unable to hash mixin data");
		}
		for (size_t i = 0; i < params->salt_size; i++) {
			params->salt[i] ^= hashed_mixin[i];
		}
		free(hashed_mixin);
	}

	return params;
}

deserialized_cleartext *
build_deserialized_cleartext_from_authenticator_parameters_and_key_spec(
    authenticator_parameters_t *authenticator_params, key_spec_t *key_spec) {
	deserialized_cleartext *cleartext = malloc(sizeof(deserialized_cleartext));
	CHECK_MALLOC(cleartext, "encrypted keyfile");
	cleartext->version = SERIALIZATION_MAX_VERSION;
	cleartext->opslimit = key_spec->opslimit;
	cleartext->memlimit = key_spec->memlimit;
	cleartext->algorithm = key_spec->algorithm;
	cleartext->kdf_salt = malloc(key_spec->kdf_salt_size);
	CHECK_MALLOC(cleartext->kdf_salt, "salt in encrypted keyfile");
	memcpy(cleartext->kdf_salt, key_spec->kdf_salt, key_spec->kdf_salt_size);
	cleartext->kdf_salt_size = key_spec->kdf_salt_size;
	cleartext->nonce = malloc(crypto_box_NONCEBYTES);
	CHECK_MALLOC(cleartext->nonce, "nonce in encrypted keyfile");
	randombytes_buf(cleartext->nonce, crypto_box_NONCEBYTES);
	cleartext->nonce_size = crypto_box_NONCEBYTES;

	deserialized_secrets *secrets = malloc(sizeof(deserialized_secrets));
	CHECK_MALLOC(secrets, "secrets");
	secrets->version = cleartext->version;
	secrets->relying_party_id = strdup(authenticator_params->relying_party_id);
	CHECK_MALLOC(secrets->relying_party_id,
	             "relying party id in encrypted keyfile")
	secrets->credential_id = malloc(authenticator_params->credential_id_size);
	CHECK_MALLOC(secrets->credential_id, "credential id in encrypted keyfile")
	memcpy(secrets->credential_id, authenticator_params->credential_id,
	       authenticator_params->credential_id_size);
	secrets->credential_id_size = authenticator_params->credential_id_size;
	secrets->salt = malloc(authenticator_params->salt_size);
	CHECK_MALLOC(secrets->salt, "encrypted secret in encrypted keyfile")
	memcpy(secrets->salt, authenticator_params->salt,
	       authenticator_params->salt_size);
	secrets->salt_size = authenticator_params->salt_size;

	cbor_item_t *cbor_encoded_secrets = serialize_secrets_to_cbor_v1(secrets);
	free_secrets(secrets);
	secrets = NULL;

	unsigned char *serialized_unencrypted_secrets;
	size_t serialized_unencrypted_secrets_size;
	if (cbor_serialize_alloc(cbor_encoded_secrets,
	                         &serialized_unencrypted_secrets,
	                         &serialized_unencrypted_secrets_size) == 0) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to serialize secrets");
	}

	cleartext->encrypted_data =
	    malloc(serialized_unencrypted_secrets_size + crypto_secretbox_MACBYTES);
	CHECK_MALLOC(cleartext->encrypted_data,
	             "encrypted data blob in encrypted keyfile");
	cleartext->encrypted_data_size =
	    serialized_unencrypted_secrets_size + crypto_secretbox_MACBYTES;
	unsigned char *key_bytes = derive_key(key_spec);
	if (crypto_secretbox_easy(cleartext->encrypted_data,
	                          serialized_unencrypted_secrets,
	                          serialized_unencrypted_secrets_size,
	                          cleartext->nonce, key_bytes) != 0) {
		errx(EXIT_CRYPTOGRAPHY_ERROR, "Could not encrypt secrets");
	}
	free(key_bytes);

	sodium_memzero(serialized_unencrypted_secrets,
	               serialized_unencrypted_secrets_size);
	free(serialized_unencrypted_secrets);
	cbor_decref(&cbor_encoded_secrets);

	return cleartext;
}

void free_cleartext(deserialized_cleartext *clear) {
	if (clear == NULL) {
		return;
	}

	if (clear->kdf_salt != NULL) {
		free(clear->kdf_salt);
	}

	if (clear->nonce != NULL) {
		free(clear->nonce);
	}

	if (clear->encrypted_data != NULL) {
		free(clear->encrypted_data);
	}

	if (clear->device_aaguid != NULL) {
		free(clear->device_aaguid);
	}

	free(clear);
}

void free_secrets(deserialized_secrets *secret) {
	if (secret == NULL) {
		return;
	}

	sodium_memzero(secret->relying_party_id, strlen(secret->relying_party_id));
	free(secret->relying_party_id);
	sodium_memzero(secret->credential_id, secret->credential_id_size);
	free(secret->credential_id);
	sodium_memzero(secret->salt, secret->salt_size);
	free(secret->salt);
	free(secret);
}

deserialized_secrets *load_secrets_from_bytes(unsigned char *decrypted,
                                              size_t decrypted_size) {
	struct cbor_load_result result;
	cbor_item_t *cbor_root = cbor_load(decrypted, decrypted_size, &result);

	if (result.error.code != CBOR_ERR_NONE) {
		errx(
		    EXIT_DESERIALIZATION_ERROR,
		    "Unable to deserialize secrets; is the data not CBOR-encoded? CBOR "
		    "error code %d: %s",
		    result.error.code, get_cbor_error_string(result.error.code));
	}

	if (!cbor_isa_array(cbor_root)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Secrets have the wrong format (should be a CBOR array at root)");
	}

	cbor_item_t *cbor_version = cbor_array_get(cbor_root, 0);
	if (!cbor_isa_uint(cbor_version) ||
	    cbor_int_get_width(cbor_version) != CBOR_INT_8) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Secrets have the wrong format (first field should be a version "
		     "number stored as an 8-bit unsigned integer)");
	}

	uint8_t version = cbor_get_uint8(cbor_version);
	cbor_decref(&cbor_version);

	switch (version) {
	case 1:
		return deserialize_secrets_from_cbor_v1(cbor_root);
	default:
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unrecognized secrets version (we only support up to %d, got "
		     "version %d)",
		     SERIALIZATION_MAX_VERSION, version);
		break;
	}
}

void write_cleartext_to_file(deserialized_cleartext *cleartext,
                             const char *path) {
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to open file at %s", path);
	}

	if (fseek(fp, 0, SEEK_SET) != 0) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to seek to start of file at %s", path);
	}

	cbor_item_t *cbor_cleartext = serialize_cleartext_to_cbor_v1(cleartext);

	unsigned char *serialized_cleartext;
	size_t serialized_cleartext_size;
	if (cbor_serialize_alloc(cbor_cleartext, &serialized_cleartext,
	                         &serialized_cleartext_size) == 0) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to serialize cleartext");
	}

	cbor_decref(&cbor_cleartext);

	if (fwrite(serialized_cleartext, serialized_cleartext_size, 1, fp) != 1) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to write file at %s", path);
	}

	free(serialized_cleartext);

	fclose(fp);
}

deserialized_cleartext *load_cleartext_from_file(const char *path) {
	long int ftell_result;

	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to open file at %s", path);
	}

	if (fseek(fp, 0, SEEK_END) != 0) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to seek to end of file at %s",
		     path);
	}

	if ((ftell_result = ftell(fp)) < 0) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to get size of file at %s",
		     path);
	}
	size_t length = (size_t)ftell_result;

	if (fseek(fp, 0, SEEK_SET) != 0) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to seek to start of file at %s", path);
	}

	if (length > LARGEST_VALID_PAYLOAD_SIZE_BYTES) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File at %s is too big (more than %d bytes); refusing to load it",
		     path, LARGEST_VALID_PAYLOAD_SIZE_BYTES);
	}

	unsigned char *buffer = malloc(length);
	CHECK_MALLOC(buffer, "buffer for reading keyfile");

	if (fread(buffer, length, 1, fp) != 1) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to read file at %s into buffer", path);
	}

	fclose(fp);

	/* Assuming `buffer` contains `info.st_size` bytes of input data */
	struct cbor_load_result result;
	cbor_item_t *cbor_root = cbor_load(buffer, length, &result);

	free(buffer);
	buffer = NULL;

	if (result.error.code != CBOR_ERR_NONE) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to deserialize file at %s; is it not CBOR-encoded? CBOR "
		     "error "
		     "code %d: %s",
		     path, result.error.code, get_cbor_error_string(result.error.code));
	}

	if (!cbor_isa_array(cbor_root)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File at %s has the wrong format (should be a CBOR array at root)",
		     path);
	}

	cbor_item_t *cbor_version = cbor_array_get(cbor_root, 0);
	if (!cbor_isa_uint(cbor_version) ||
	    cbor_int_get_width(cbor_version) != CBOR_INT_8) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File at %s has the wrong format (first field should be a version "
		     "number stored as an 8-bit unsigned integer)",
		     path);
	}

	uint8_t version = cbor_get_uint8(cbor_version);
	cbor_decref(&cbor_version);

	switch (version) {
	case 1:
		return deserialize_cleartext_from_cbor_v1(cbor_root);
	default:
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unrecognized file version (we only support up to %d, got version "
		     "%d)",
		     SERIALIZATION_MAX_VERSION, version);
		break;
	}
}

const char *get_cbor_error_string(cbor_error_code code) {
	switch (code) {
	case CBOR_ERR_MALFORMATED:
		return "malformed data (CBOR_ERR_MALFORMATED)";
	case CBOR_ERR_MEMERROR:
		return "memory error (CBOR_ERR_MEMERROR)";
	case CBOR_ERR_NODATA:
		return "no data (CBOR_ERR_NODATA)";
	case CBOR_ERR_NOTENOUGHDATA:
		return "not enough data (CBOR_ERR_NOTENOUGHDATA)";
	case CBOR_ERR_SYNTAXERROR:
		return "syntax error in data (CBOR_ERR_SYNTAXERROR)";
	default:
		return "unknown";
	}
}

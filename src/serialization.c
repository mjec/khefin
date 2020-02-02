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
#include "memory.h"

authenticator_parameters_t *
build_authenticator_parameters_from_deserialized_cleartext_and_key_and_mixin(
    deserialized_cleartext *cleartext, unsigned char *key_bytes, char *mixin) {
	unsigned char *decrypted = malloc_or_exit(cleartext->encrypted_data_size -
	                                              crypto_secretbox_MACBYTES,
	                                          "encrypted data");
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
	params->relying_party_id =
	    strdup_or_exit(secrets->relying_party_id,
	                   "relying party id in authenticator parameters");
	memcpy(params->salt, secrets->salt, secrets->salt_size);
	free_secrets(secrets);
	secrets = NULL;

	if (mixin != NULL) {
		unsigned char *hashed_mixin =
		    malloc_or_exit(params->salt_size, "mixin salt");
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
	deserialized_cleartext *cleartext =
	    malloc_or_exit(sizeof(deserialized_cleartext), "encrypted keyfile");
	cleartext->version = SERIALIZATION_MAX_VERSION;
	cleartext->opslimit = key_spec->opslimit;
	cleartext->memlimit = key_spec->memlimit;
	cleartext->algorithm = key_spec->algorithm;
	cleartext->kdf_salt =
	    malloc_or_exit(key_spec->kdf_salt_size, "salt in encrypted keyfile");
	memcpy(cleartext->kdf_salt, key_spec->kdf_salt, key_spec->kdf_salt_size);
	cleartext->kdf_salt_size = key_spec->kdf_salt_size;
	cleartext->nonce =
	    malloc_or_exit(crypto_box_NONCEBYTES, "nonce in encrypted keyfile");
	randombytes_buf(cleartext->nonce, crypto_box_NONCEBYTES);
	cleartext->nonce_size = crypto_box_NONCEBYTES;

	deserialized_secrets *secrets =
	    malloc_or_exit(sizeof(deserialized_secrets), "secrets");
	secrets->version = cleartext->version;
	secrets->relying_party_id =
	    strdup_or_exit(authenticator_params->relying_party_id,
	                   "relying party id in encrypted keyfile");
	secrets->credential_id =
	    malloc_or_exit(authenticator_params->credential_id_size,
	                   "credential id in encrypted keyfile");
	memcpy(secrets->credential_id, authenticator_params->credential_id,
	       authenticator_params->credential_id_size);
	secrets->credential_id_size = authenticator_params->credential_id_size;
	secrets->salt = malloc_or_exit(authenticator_params->salt_size,
	                               "encrypted secret in encrypted keyfile");
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

	cleartext->encrypted_data = malloc_or_exit(
	    serialized_unencrypted_secrets_size + crypto_secretbox_MACBYTES,
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

encoded_file *write_cleartext(deserialized_cleartext *cleartext,
                              const char *path) {
	encoded_file *result = malloc_or_exit(sizeof(encoded_file), "encoded file structure");
	result->path = strdup_or_exit(path, "encoded file path");

	cbor_item_t *cbor_cleartext = serialize_cleartext_to_cbor_v1(cleartext);

	if (cbor_serialize_alloc(cbor_cleartext, &result->data, &result->length) ==
	    0) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to serialize cleartext");
	}

	cbor_decref(&cbor_cleartext);

	return result;
}

deserialized_cleartext *load_cleartext(encoded_file *file) {
	struct cbor_load_result result;
	cbor_item_t *cbor_root = cbor_load(file->data, file->length, &result);

	if (result.error.code != CBOR_ERR_NONE) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to deserialize %s; is it not CBOR-encoded? CBOR error "
		     "code %d: %s",
		     file->path, result.error.code,
		     get_cbor_error_string(result.error.code));
	}

	if (!cbor_isa_array(cbor_root)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "%s has the wrong format (should be a CBOR array at root)",
		     file->path);
	}

	cbor_item_t *cbor_version = cbor_array_get(cbor_root, 0);
	if (!cbor_isa_uint(cbor_version) ||
	    cbor_int_get_width(cbor_version) != CBOR_INT_8) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "%s has the wrong format (first field should be a version number "
		     "stored as an 8-bit unsigned integer)",
		     file->path);
	}

	uint8_t version = cbor_get_uint8(cbor_version);
	cbor_decref(&cbor_version);

	switch (version) {
	case 1:
		return deserialize_cleartext_from_cbor_v1(cbor_root);
	default:
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unrecognized data version (we only support up to %d, got version "
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

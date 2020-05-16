#include "serialization/v1.h"

#include <sodium.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "exit.h"
#include "memory.h"

deserialized_cleartext *
deserialize_cleartext_from_cbor_v1(cbor_item_t *cbor_root) {
	deserialized_cleartext *clear =
	    malloc_or_exit(sizeof(deserialized_cleartext), "keyfile");

	if (cbor_array_size(cbor_root) != CLEAR_COUNT_OF_FIELDS) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format for v1 (should be a CBOR array with %d "
		     "elements at root)",
		     CLEAR_COUNT_OF_FIELDS);
	}

	cbor_item_t *cbor_version = cbor_array_get(cbor_root, CLEAR_FIELD_VERSION);
	if (!cbor_isa_uint(cbor_version) ||
	    cbor_int_get_width(cbor_version) != CBOR_INT_8) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a version number "
		     "stored as an 8-bit unsigned integer)",
		     CLEAR_FIELD_VERSION);
	}
	clear->version = cbor_get_uint8(cbor_version);
	if (clear->version != SERIALIZATION_VERSION) {
		errx(EXIT_PROGRAMMER_ERROR,
		     "BUG (%s:%d): deserialize_cleartext_from_cbor_v1() called when "
		     "file version is not %d (version is %d)",
		     __func__, __LINE__, SERIALIZATION_VERSION, clear->version);
	}
	cbor_decref(&cbor_version);
	cbor_version = NULL;

	cbor_item_t *cbor_device_aaguid =
	    cbor_array_get(cbor_root, CLEAR_FIELD_DEVICE_AAGUID);
	if (!cbor_isa_bytestring(cbor_device_aaguid) ||
	    !cbor_bytestring_is_definite(cbor_device_aaguid)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a AAGUID as a "
		     "definite bytestring)",
		     CLEAR_FIELD_DEVICE_AAGUID);
	}
	clear->device_aaguid_size = cbor_bytestring_length(cbor_device_aaguid);
	if (clear->device_aaguid_size > 0) {
		clear->device_aaguid = malloc_or_exit(clear->device_aaguid_size,
		                                      "device AAGUID in keyfile");
		memcpy(clear->device_aaguid, cbor_bytestring_handle(cbor_device_aaguid),
		       clear->device_aaguid_size);
	} else {
		clear->device_aaguid = NULL;
	}
	cbor_decref(&cbor_device_aaguid);
	cbor_device_aaguid = NULL;

	cbor_item_t *cbor_kdf_salt =
	    cbor_array_get(cbor_root, CLEAR_FIELD_KDF_SALT);
	if (!cbor_isa_bytestring(cbor_kdf_salt) ||
	    !cbor_bytestring_is_definite(cbor_kdf_salt)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a salt as a "
		     "definite bytestring)",
		     CLEAR_FIELD_KDF_SALT);
	}
	clear->kdf_salt_size = cbor_bytestring_length(cbor_kdf_salt);
	if (clear->kdf_salt_size > 0) {
		clear->kdf_salt =
		    malloc_or_exit(clear->kdf_salt_size, "salt in keyfile");
		memcpy(clear->kdf_salt, cbor_bytestring_handle(cbor_kdf_salt),
		       clear->kdf_salt_size);
	} else {
		clear->kdf_salt = NULL;
	}
	cbor_decref(&cbor_kdf_salt);
	cbor_kdf_salt = NULL;

	cbor_item_t *cbor_opslimit =
	    cbor_array_get(cbor_root, CLEAR_FIELD_OPSLIMIT);
	if (!cbor_isa_uint(cbor_opslimit) ||
	    cbor_int_get_width(cbor_opslimit) != CBOR_INT_64) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a 64-bit unsigned "
		     "integer)",
		     CLEAR_FIELD_OPSLIMIT);
	}
	clear->opslimit = (unsigned long long)cbor_get_uint64(cbor_opslimit);
	cbor_decref(&cbor_opslimit);
	cbor_opslimit = NULL;

	cbor_item_t *cbor_memlimit =
	    cbor_array_get(cbor_root, CLEAR_FIELD_MEMLIMIT);
	if (!cbor_isa_uint(cbor_memlimit) ||
	    cbor_int_get_width(cbor_memlimit) != CBOR_INT_64) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a 64-bit unsigned "
		     "integer)",
		     CLEAR_FIELD_MEMLIMIT);
	}
	clear->memlimit = (size_t)cbor_get_uint64(cbor_memlimit);
	cbor_decref(&cbor_memlimit);
	cbor_memlimit = NULL;

	cbor_item_t *cbor_algorithm =
	    cbor_array_get(cbor_root, CLEAR_FIELD_ALGORITHM);
	if (!cbor_isa_uint(cbor_algorithm) ||
	    cbor_int_get_width(cbor_algorithm) != CBOR_INT_16) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a 16-bit unsigned "
		     "integer)",
		     CLEAR_FIELD_ALGORITHM);
	}
	clear->algorithm = (int)cbor_get_uint16(cbor_algorithm);
	cbor_decref(&cbor_algorithm);
	cbor_algorithm = NULL;

	cbor_item_t *cbor_nonce = cbor_array_get(cbor_root, CLEAR_FIELD_NONCE);
	if (!cbor_isa_bytestring(cbor_nonce) ||
	    !cbor_bytestring_is_definite(cbor_nonce)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File has the wrong format (field %d should be a nonce as a "
		     "definite bytestring)",
		     CLEAR_FIELD_NONCE);
	}
	clear->nonce_size = cbor_bytestring_length(cbor_nonce);
	if (clear->nonce_size > 0) {
		clear->nonce = malloc_or_exit(clear->nonce_size, "nonce in keyfile");
		memcpy(clear->nonce, cbor_bytestring_handle(cbor_nonce),
		       clear->nonce_size);
	} else {
		clear->nonce = NULL;
	}
	cbor_decref(&cbor_nonce);
	cbor_nonce = NULL;

	cbor_item_t *cbor_encrypted_data =
	    cbor_array_get(cbor_root, CLEAR_FIELD_ENCRYPTED_DATA);
	if (!cbor_isa_bytestring(cbor_encrypted_data) ||
	    !cbor_bytestring_is_definite(cbor_encrypted_data)) {
		errx(
		    EXIT_DESERIALIZATION_ERROR,
		    "File has the wrong format (field %d should be encrypted data as a "
		    "definite bytestring)",
		    CLEAR_FIELD_ENCRYPTED_DATA);
	}
	clear->encrypted_data_size = cbor_bytestring_length(cbor_encrypted_data);
	if (clear->encrypted_data_size > 0) {
		clear->encrypted_data = malloc_or_exit(
		    clear->encrypted_data_size, "encrypted data blob in keyfile");
		memcpy(clear->encrypted_data,
		       cbor_bytestring_handle(cbor_encrypted_data),
		       clear->encrypted_data_size);
	} else {
		clear->encrypted_data = NULL;
	}
	cbor_decref(&cbor_encrypted_data);
	cbor_encrypted_data = NULL;

	cbor_decref(&cbor_root);
	return clear;
}

cbor_item_t *serialize_cleartext_to_cbor_v1(deserialized_cleartext *clear) {
	cbor_item_t *root = cbor_new_definite_array(CLEAR_COUNT_OF_FIELDS);

	FIELD_COUNTER_ASSERT_START;

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_VERSION, __LINE__);
	cbor_item_t *version = cbor_build_uint8(clear->version);
	cbor_array_push(root, version);
	cbor_decref(&version);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_DEVICE_AAGUID, __LINE__);
	cbor_item_t *device_aaguid =
	    cbor_build_bytestring(clear->device_aaguid, clear->device_aaguid_size);
	cbor_array_push(root, device_aaguid);
	cbor_decref(&device_aaguid);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_KDF_SALT, __LINE__);
	cbor_item_t *salt =
	    cbor_build_bytestring(clear->kdf_salt, clear->kdf_salt_size);
	cbor_array_push(root, salt);
	cbor_decref(&salt);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_OPSLIMIT, __LINE__);
	cbor_item_t *opslimit = cbor_build_uint64(clear->opslimit);
	cbor_array_push(root, opslimit);
	cbor_decref(&opslimit);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_MEMLIMIT, __LINE__);
	cbor_item_t *memlimit = cbor_build_uint64(clear->memlimit);
	cbor_array_push(root, memlimit);
	cbor_decref(&memlimit);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_ALGORITHM, __LINE__);
	cbor_item_t *algorithm = cbor_build_uint16(clear->algorithm);
	cbor_array_push(root, algorithm);
	cbor_decref(&algorithm);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_NONCE, __LINE__);
	cbor_item_t *nonce = cbor_build_bytestring(clear->nonce, clear->nonce_size);
	cbor_array_push(root, nonce);
	cbor_decref(&nonce);

	FIELD_COUNTER_ASSERT(__func__, CLEAR_FIELD_ENCRYPTED_DATA, __LINE__);
	cbor_item_t *encrypted_data = cbor_build_bytestring(
	    clear->encrypted_data, clear->encrypted_data_size);
	cbor_array_push(root, encrypted_data);
	cbor_decref(&encrypted_data);

	return root;
}

deserialized_secrets *deserialize_secrets_from_cbor_v1(cbor_item_t *cbor_root) {
	deserialized_secrets *secrets =
	    malloc_or_exit(sizeof(deserialized_secrets), "decrypted secret blob");

	if (cbor_array_size(cbor_root) != ENCRYPTED_COUNT_OF_FIELDS) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Decrypted blob has the wrong format for v1 (should be a CBOR "
		     "array with %d elements at root)",
		     ENCRYPTED_COUNT_OF_FIELDS);
	}

	cbor_item_t *cbor_version =
	    cbor_array_get(cbor_root, ENCRYPTED_FIELD_VERSION);
	if (!cbor_isa_uint(cbor_version) ||
	    cbor_int_get_width(cbor_version) != CBOR_INT_8) {
		errx(
		    EXIT_DESERIALIZATION_ERROR,
		    "Decrypted blob has the wrong format (field %d should be a version "
		    "number stored as an 8-bit unsigned integer)",
		    ENCRYPTED_FIELD_VERSION);
	}
	secrets->version = cbor_get_uint8(cbor_version);
	if (secrets->version != SERIALIZATION_VERSION) {
		errx(EXIT_PROGRAMMER_ERROR,
		     "BUG (%s:%d): deserialize_secrets_from_cbor_v1() called when file "
		     "version is not %d (version is %d)",
		     __func__, __LINE__, SERIALIZATION_VERSION, secrets->version);
	}
	cbor_decref(&cbor_version);
	cbor_version = NULL;

	cbor_item_t *cbor_relying_party_id =
	    cbor_array_get(cbor_root, ENCRYPTED_FIELD_RP_ID);
	if (!cbor_isa_string(cbor_relying_party_id) ||
	    !cbor_string_is_definite(cbor_relying_party_id)) {
		errx(
		    EXIT_DESERIALIZATION_ERROR,
		    "Decrypted blob has the wrong format (field %d should be a relying "
		    "party ID as a definite UTF-8 string)",
		    ENCRYPTED_FIELD_RP_ID);
	}
	size_t relying_party_size = cbor_string_length(cbor_relying_party_id);
	if (relying_party_size > 0) {
		secrets->relying_party_id =
		    malloc_or_exit(relying_party_size + 1,
		                   "relying party id in decrypted secret blob");
		strncpy(secrets->relying_party_id,
		        (const char *restrict)cbor_string_handle(cbor_relying_party_id),
		        relying_party_size); // We explicitly add the null terminator in
		                             // the next line
		secrets->relying_party_id[relying_party_size] = (char)0;
	} else {
		secrets->relying_party_id = NULL;
	}
	cbor_decref(&cbor_relying_party_id);
	cbor_relying_party_id = NULL;

	cbor_item_t *cbor_credential_id =
	    cbor_array_get(cbor_root, ENCRYPTED_FIELD_CREDENTIAL_ID);
	if (!cbor_isa_bytestring(cbor_credential_id) ||
	    !cbor_bytestring_is_definite(cbor_credential_id)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Decrypted blob has the wrong format (field %d should be a "
		     "credential ID as a definite bytestring)",
		     ENCRYPTED_FIELD_CREDENTIAL_ID);
	}
	secrets->credential_id_size = cbor_bytestring_length(cbor_credential_id);
	if (secrets->credential_id_size > 0) {
		secrets->credential_id =
		    malloc_or_exit(secrets->credential_id_size,
		                   "credential id in decrypted secret blob");
		memcpy(secrets->credential_id,
		       cbor_bytestring_handle(cbor_credential_id),
		       secrets->credential_id_size);
	} else {
		secrets->credential_id = NULL;
	}
	cbor_decref(&cbor_credential_id);
	cbor_credential_id = NULL;

	cbor_item_t *cbor_salt =
	    cbor_array_get(cbor_root, ENCRYPTED_FIELD_HMAC_SALT);
	if (!cbor_isa_bytestring(cbor_salt) ||
	    !cbor_bytestring_is_definite(cbor_salt)) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Decrypted blob has the wrong format (field %d should be a HMAC "
		     "salt as a definite bytestring)",
		     ENCRYPTED_FIELD_HMAC_SALT);
	}
	secrets->salt_size = cbor_bytestring_length(cbor_salt);
	if (secrets->salt_size > 0) {
		secrets->salt =
		    malloc_or_exit(secrets->salt_size, "salt in decrypted secret blob");
		memcpy(secrets->salt, cbor_bytestring_handle(cbor_salt),
		       secrets->salt_size);
	} else {
		secrets->salt = NULL;
	}
	cbor_decref(&cbor_salt);
	cbor_salt = NULL;

	cbor_decref(&cbor_root);

	return secrets;
}

cbor_item_t *serialize_secrets_to_cbor_v1(deserialized_secrets *secrets) {
	cbor_item_t *root = cbor_new_definite_array(ENCRYPTED_COUNT_OF_FIELDS);

	FIELD_COUNTER_ASSERT_START;

	FIELD_COUNTER_ASSERT(__func__, ENCRYPTED_FIELD_VERSION, __LINE__);
	cbor_item_t *version = cbor_build_uint8(secrets->version);
	cbor_array_push(root, version);
	cbor_decref(&version);

	FIELD_COUNTER_ASSERT(__func__, ENCRYPTED_FIELD_RP_ID, __LINE__);
	cbor_item_t *relying_party_id =
	    cbor_build_string(secrets->relying_party_id);
	cbor_array_push(root, relying_party_id);
	cbor_decref(&relying_party_id);

	FIELD_COUNTER_ASSERT(__func__, ENCRYPTED_FIELD_CREDENTIAL_ID, __LINE__);
	cbor_item_t *credential_id = cbor_build_bytestring(
	    secrets->credential_id, secrets->credential_id_size);
	cbor_array_push(root, credential_id);
	cbor_decref(&credential_id);

	FIELD_COUNTER_ASSERT(__func__, ENCRYPTED_FIELD_HMAC_SALT, __LINE__);
	cbor_item_t *salt =
	    cbor_build_bytestring(secrets->salt, secrets->salt_size);
	cbor_array_push(root, salt);
	cbor_decref(&salt);

	return root;
}

#include "serialization/v1.h"

#include <sodium.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "exit.h"

deserialized_cleartext *
deserialize_cleartext_from_cbor_v1(cbor_item_t *cbor_root) {
	deserialized_cleartext *clear = malloc(sizeof(deserialized_cleartext));
	if (clear == NULL) {
		errx(EXIT_OUT_OF_MEMORY, "Out of memory attempting to allocate "
		                         "deserialized cleartext struct");
	}

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
		     __FILE__, __LINE__, SERIALIZATION_VERSION, clear->version);
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
		clear->device_aaguid = malloc(clear->device_aaguid_size);
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
		clear->kdf_salt = malloc(clear->kdf_salt_size);
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
		clear->nonce = malloc(clear->nonce_size);
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
		clear->encrypted_data = malloc(clear->encrypted_data_size);
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

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1", CLEAR_FIELD_VERSION,
	                     __FILE__, __LINE__);
	cbor_item_t *version = cbor_build_uint8(clear->version);
	cbor_array_push(root, version);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1",
	                     CLEAR_FIELD_DEVICE_AAGUID, __FILE__, __LINE__);
	cbor_item_t *device_aaguid =
	    cbor_build_bytestring(clear->device_aaguid, clear->device_aaguid_size);
	cbor_array_push(root, device_aaguid);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1", CLEAR_FIELD_KDF_SALT,
	                     __FILE__, __LINE__);
	cbor_item_t *salt =
	    cbor_build_bytestring(clear->kdf_salt, clear->kdf_salt_size);
	cbor_array_push(root, salt);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1", CLEAR_FIELD_OPSLIMIT,
	                     __FILE__, __LINE__);
	cbor_item_t *opslimit = cbor_build_uint64(clear->opslimit);
	cbor_array_push(root, opslimit);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1", CLEAR_FIELD_MEMLIMIT,
	                     __FILE__, __LINE__);
	cbor_item_t *memlimit = cbor_build_uint64(clear->memlimit);
	cbor_array_push(root, memlimit);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1",
	                     CLEAR_FIELD_ALGORITHM, __FILE__, __LINE__);
	cbor_item_t *algorithm = cbor_build_uint16(clear->algorithm);
	cbor_array_push(root, algorithm);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1", CLEAR_FIELD_NONCE,
	                     __FILE__, __LINE__);
	cbor_item_t *nonce = cbor_build_bytestring(clear->nonce, clear->nonce_size);
	cbor_array_push(root, nonce);

	FIELD_COUNTER_ASSERT("serialize_cleartext_to_cbor_v1",
	                     CLEAR_FIELD_ENCRYPTED_DATA, __FILE__, __LINE__);
	cbor_item_t *encrypted_data = cbor_build_bytestring(
	    clear->encrypted_data, clear->encrypted_data_size);
	cbor_array_push(root, encrypted_data);

	return root;
}

deserialized_secrets *deserialize_secrets_from_cbor_v1(cbor_item_t *cbor_root) {
	deserialized_secrets *secrets = malloc(sizeof(deserialized_secrets));
	if (secrets == NULL) {
		errx(
		    EXIT_OUT_OF_MEMORY,
		    "Out of memory attempting to allocate deserialized secrets struct");
	}

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
		     __FILE__, __LINE__, SERIALIZATION_VERSION, secrets->version);
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
		secrets->relying_party_id = malloc(relying_party_size + 1);
		strncpy(secrets->relying_party_id,
		        (const char *restrict)cbor_string_handle(cbor_relying_party_id),
		        relying_party_size + 1);
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
		secrets->credential_id = malloc(secrets->credential_id_size);
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
		secrets->salt = malloc(secrets->salt_size);
		memcpy(secrets->salt, cbor_bytestring_handle(cbor_salt),
		       secrets->salt_size);
	} else {
		secrets->salt = NULL;
	}
	cbor_decref(&cbor_salt);
	cbor_salt = NULL;

	return secrets;
}

cbor_item_t *serialize_secrets_to_cbor_v1(deserialized_secrets *secrets) {
	cbor_item_t *root = cbor_new_definite_array(ENCRYPTED_COUNT_OF_FIELDS);

	FIELD_COUNTER_ASSERT_START;

	FIELD_COUNTER_ASSERT("serialize_secrets_to_cbor_v1",
	                     ENCRYPTED_FIELD_VERSION, __FILE__, __LINE__);
	cbor_item_t *version = cbor_build_uint8(secrets->version);
	cbor_array_push(root, version);

	FIELD_COUNTER_ASSERT("serialize_secrets_to_cbor_v1", ENCRYPTED_FIELD_RP_ID,
	                     __FILE__, __LINE__);
	cbor_item_t *relying_party_id =
	    cbor_build_string(secrets->relying_party_id);
	cbor_array_push(root, relying_party_id);

	FIELD_COUNTER_ASSERT("serialize_secrets_to_cbor_v1",
	                     ENCRYPTED_FIELD_CREDENTIAL_ID, __FILE__, __LINE__);
	cbor_item_t *credential_id = cbor_build_bytestring(
	    secrets->credential_id, secrets->credential_id_size);
	cbor_array_push(root, credential_id);

	FIELD_COUNTER_ASSERT("serialize_secrets_to_cbor_v1",
	                     ENCRYPTED_FIELD_HMAC_SALT, __FILE__, __LINE__);
	cbor_item_t *salt =
	    cbor_build_bytestring(secrets->salt, secrets->salt_size);
	cbor_array_push(root, salt);

	return root;
}

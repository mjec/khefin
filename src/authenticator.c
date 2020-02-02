#include "authenticator.h"

#include <fido.h>
#include <sodium.h>
#include <string.h>

#include "exit.h"
#include "memory.h"
#include "serialization.h"

devices_list_t *list_devices(void) {
	fido_dev_info_t *list;
	size_t count;
	int r;
	devices_list_t *result =
	    malloc_or_exit(sizeof(devices_list_t), "list of devices");

	if ((list = fido_dev_info_new(MAX_DEVICES_TO_LIST)) == NULL) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to create new list of devices");
	}

	if ((r = fido_dev_info_manifest(list, MAX_DEVICES_TO_LIST, &count)) !=
	    FIDO_OK) {
		if (r != FIDO_ERR_INTERNAL) {
			errx(EXIT_AUTHENTICATOR_ERROR,
			     "Unable to get devices manifest: %s (0x%x)", fido_strerr(r),
			     r);
		}
		count = 0;
		list = NULL;
	}

	result->count = count;
	result->list = list;

	return result;
}

void free_devices_list(devices_list_t *devices_list) {
	if (devices_list == NULL) {
		return;
	}
	if (devices_list->list != NULL) {
		fido_dev_info_free(&devices_list->list, devices_list->count);
	}
	free(devices_list);
}

fido_dev_t *get_device(const char *path) {
	fido_dev_t *device;
	int r;

	if ((device = fido_dev_new()) == NULL) {
		errx(EXIT_OUT_OF_MEMORY,
		     "Unable to create device structure (out of memory?)");
	}

	if ((r = fido_dev_open(device, path)) != FIDO_OK) {
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to access device at %s: %s (0x%x)", path, fido_strerr(r),
		     r);
	}

	if (fido_dev_is_fido2(device) == false) {
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Device at %s is not a FIDO2 authenticator", path);
	}

	return device;
}

bool device_supports_hmac_secret(fido_cbor_info_t *device_info) {
	char *const *extensions = fido_cbor_info_extensions_ptr(device_info);
	size_t extensions_length = fido_cbor_info_extensions_len(device_info);

	for (size_t i = 0; i < extensions_length; i++) {
		if (strcmp("hmac-secret", extensions[i]) == 0) {
			return true;
		}
	}

	return false;
}

fido_cbor_info_t *get_device_info(fido_dev_t *device) {
	fido_cbor_info_t *device_info;
	int r;

	if ((device_info = fido_cbor_info_new()) == NULL) {
		errx(EXIT_OUT_OF_MEMORY,
		     "Unable to create device info structure (out of memory?)");
	}

	if ((r = fido_dev_get_cbor_info(device, device_info)) != FIDO_OK) {
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to get info from device: %s (0x%x)", fido_strerr(r), r);
	}

	return device_info;
}

void free_device_info(fido_cbor_info_t *cbor_info) {
	if (cbor_info) {
		fido_cbor_info_free(&cbor_info);
	}
}

void close_and_free_device_ignoring_errors(fido_dev_t *device) {
	if (device == NULL) {
		return;
	}
	int r;
	if ((r = fido_dev_close(device)) != FIDO_OK) {
		warnx("Unable to close device: %s (0x%x)", fido_strerr(r), r);
		return;
	};
	fido_dev_free(&device);
}

void close_and_free_device(fido_dev_t *device) {
	if (device == NULL) {
		return;
	}

	int r;

	if ((r = fido_dev_close(device)) != FIDO_OK) {
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to close device: %s (0x%x)",
		     fido_strerr(r), r);
	}

	fido_dev_free(&device);
}

authenticator_parameters_t *
allocate_parameters_except_rpid(size_t credential_id_size, size_t salt_size) {
	authenticator_parameters_t *params = malloc_or_exit(
	    sizeof(authenticator_parameters_t), "authenticator parameters");

	if (credential_id_size) {
		params->credential_id = malloc_or_exit(
		    credential_id_size, "credential id in authenticator parameters");
		params->credential_id_size = credential_id_size;
	} else {
		params->credential_id = NULL;
		params->credential_id_size = 0;
	}

	if (salt_size) {
		params->salt =
		    malloc_or_exit(salt_size, "salt in authenticator parameters");
		params->salt_size = salt_size;
	} else {
		params->salt = NULL;
		params->salt_size = 0;
	}

	// This data is required, but isn't meaninfully used, so we zero it out
	params->user_id = calloc(1, 1);
	params->user_id_size = 1;

	// CDH must be 32 bytes
	params->client_data_hash =
	    malloc_or_exit(CLIENT_DATA_HASH_SIZE_BYTES,
	                   "client data hash in authenticator parameters");
	params->client_data_hash_size = CLIENT_DATA_HASH_SIZE_BYTES;
	for (int i = 0; i < params->client_data_hash_size; i++) {
		params->client_data_hash[i] = 'A';
	}

	// Empty null-terminated UTF-8 strings
	params->user_name = calloc(2, 1);
	params->user_name[0] = 'A';
	params->user_display_name = calloc(2, 1);
	params->user_display_name[0] = 'A';
	params->relying_party_name = calloc(2, 1);
	params->relying_party_name[0] = 'A';

	return params;
}

/**
 * This function assumes all pointers are to malloc()'d memory
 * except relying_party_id, credential_id and salt, which must
 * be malloc()'d.
 */
void free_parameters(authenticator_parameters_t *params) {
	if (params == NULL) {
		return;
	}

	if (params->user_id != NULL) {
		free(params->user_id);
	}

	if (params->client_data_hash != NULL) {
		free(params->client_data_hash);
	}

	if (params->user_name != NULL) {
		free(params->user_name);
	}

	if (params->user_display_name != NULL) {
		free(params->user_display_name);
	}

	if (params->relying_party_name != NULL) {
		free(params->relying_party_name);
	}

	// The following parameters are secret
	if (params->relying_party_id != NULL) {
		sodium_memzero(params->relying_party_id, RELYING_PARTY_ID_SIZE);
		free(params->relying_party_id);
	}
	if (params->credential_id != NULL) {
		sodium_memzero(params->credential_id, params->credential_id_size);
		free(params->credential_id);
	}
	if (params->salt != NULL) {
		sodium_memzero(params->salt, params->salt_size);
		free(params->salt);
	}
	free(params);
}

void create_credential(fido_dev_t *device, authenticator_parameters_t *params) {
	fido_cred_t *credential;
	const unsigned char *cred_id;
	size_t cred_id_size;
	int r;

	if ((credential = fido_cred_new()) == NULL) {
		errx(EXIT_OUT_OF_MEMORY,
		     "Unable to create credential structure (out of memory?)");
	}

	if ((r = fido_cred_set_extensions(credential, FIDO_EXT_HMAC_SECRET)) !=
	    FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to set extension to hmac-secret: %s (0x%x)",
		     fido_strerr(r), r);
	}

	if ((r = fido_cred_set_rp(credential, params->relying_party_id,
	                          params->relying_party_name)) != FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to set relying party: %s (0x%x)",
		     fido_strerr(r), r);
	}

	if ((r = fido_cred_set_type(credential, COSE_ES256)) != FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to set credential type: %s (0x%x)", fido_strerr(r), r);
	}

	if ((r = fido_cred_set_user(credential, params->user_id,
	                            params->user_id_size, params->user_name,
	                            params->user_display_name, NULL)) != FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to set user info: %s (0x%x)",
		     fido_strerr(r), r);
	}

	if ((r = fido_cred_set_clientdata_hash(credential, params->client_data_hash,
	                                       params->client_data_hash_size)) !=
	    FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to set client data hash: %s (0x%x)", fido_strerr(r), r);
	}

	if ((r = fido_cred_set_rk(credential, FIDO_OPT_FALSE)) != FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to disable use of resident key: %s (0x%x)", fido_strerr(r),
		     r);
	}

	// TODO: PIN support
	if ((r = fido_dev_make_cred(device, credential, NULL)) != FIDO_OK) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to create credential on FIDO2 device: %s (0x%x)",
		     fido_strerr(r), r);
	}

	cred_id = fido_cred_id_ptr(credential);
	cred_id_size = fido_cred_id_len(credential);

	if (cred_id == NULL || cred_id_size < 1) {
		fido_cred_free(&credential);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to read credential ID");
	}

	params->credential_id = malloc_or_exit(
	    cred_id_size, "credential id in authenticator parameters");
	params->credential_id_size = cred_id_size;
	memcpy(params->credential_id, cred_id, cred_id_size);

	fido_cred_free(&credential);
}

int get_secret_from_authenticator_params(
    fido_dev_t *device, authenticator_parameters_t *params,
    secret_t *secret_struct) { // cred_id_t *cred_struct, unsigned char *salt,
	                           // size_t salt_length) {
	int r;
	const unsigned char *secret_pointer;
	size_t secret_size;
	fido_assert_t *assertion;

	if (params->credential_id == NULL || params->credential_id_size < 1) {
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_PROGRAMMER_ERROR, "BUG (%s:%d): No credential ID supplied",
		     __func__, __LINE__);
	}

	if (params->salt == NULL || params->salt_size < 1) {
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_PROGRAMMER_ERROR, "BUG (%s:%d): No salt supplied", __func__,
		     __LINE__);
	}

	if ((assertion = fido_assert_new()) == NULL) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_OUT_OF_MEMORY,
		     "Unable to create assertion structure (out of memory?)");
	}

	if ((r = fido_assert_set_hmac_salt(assertion, params->salt,
	                                   params->salt_size)) != FIDO_OK) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to set extension to HMAC salt: %s (0x%x)", fido_strerr(r),
		     r);
	}

	if ((r = fido_assert_set_extensions(assertion, FIDO_EXT_HMAC_SECRET)) !=
	    FIDO_OK) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to set extension to hmac-secret: %s (0x%x)",
		     fido_strerr(r), r);
	}

	if ((r = fido_assert_set_rp(assertion, params->relying_party_id)) !=
	    FIDO_OK) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to set relying party: %s (0x%x)",
		     fido_strerr(r), r);
	}

	if ((r = fido_assert_set_clientdata_hash(
	         assertion, params->client_data_hash,
	         params->client_data_hash_size)) != FIDO_OK) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to set client data hash: %s (0x%x)", fido_strerr(r), r);
	}

	if ((r = fido_assert_allow_cred(assertion, params->credential_id,
	                                params->credential_id_size)) != FIDO_OK) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to set credential ID: %s (0x%x)",
		     fido_strerr(r), r);
	}

	if ((r = fido_assert_set_up(assertion, FIDO_OPT_TRUE)) != FIDO_OK) {
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to run assert_set_up(): %s (0x%x)", fido_strerr(r), r);
	}

	// TODO: PIN support
	r = fido_dev_get_assert(device, assertion, NULL);
	if (r != FIDO_OK) {
		fido_assert_free(&assertion);
		if (r == FIDO_ERR_INVALID_CREDENTIAL ||
		    r == FIDO_ERR_USER_ACTION_PENDING || r == FIDO_ERR_NO_CREDENTIALS ||
		    r == FIDO_ERR_ACTION_TIMEOUT) {
			return r;
		}
		free_parameters(params);
		close_and_free_device_ignoring_errors(device);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Unable to get secret from device: %s (0x%x)", fido_strerr(r), r);
	}

	secret_pointer = fido_assert_hmac_secret_ptr(assertion, 0);
	secret_size = fido_assert_hmac_secret_len(assertion, 0);

	if (secret_pointer == NULL || secret_size < 1) {
		close_and_free_device_ignoring_errors(device);
		fido_assert_free(&assertion);
		errx(EXIT_AUTHENTICATOR_ERROR, "Unable to read credential ID");
	}

	secret_struct->secret_size = secret_size;
	secret_struct->secret =
	    malloc_or_exit(secret_struct->secret_size, "secret");
	memcpy(secret_struct->secret, secret_pointer, secret_size);
	fido_assert_free(&assertion);

	return FIDO_OK;
}

void free_secret(secret_t *secret_struct) {
	if (secret_struct == NULL) {
		return;
	}
	free(secret_struct->secret);
	free(secret_struct);
}

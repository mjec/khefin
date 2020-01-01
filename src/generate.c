#include "generate.h"

#include <sodium.h>
#include <stdio.h>
#include <string.h>

#include "exit.h"
#include "serialization.h"

unsigned short int print_secret(invocation_state_t *invocation,
                                devices_list_t *devices_list) {
	deserialized_cleartext *cleartext =
	    load_cleartext_from_file(invocation->file);
	authenticator_parameters_t *authenticator_params =
	    build_authenticator_parameters_from_deserialized_cleartext_and_passphrase(
	        cleartext, invocation->passphrase);

	// Clean up things we don't need anymore
	sodium_memzero(invocation->passphrase, strlen(invocation->passphrase));
	free(invocation->device);
	free(invocation->file);
	free(invocation->passphrase);
	free(invocation);
	invocation = NULL;

	for (size_t i = 0; i < devices_list->count; i++) {
		const fido_dev_info_t *di = fido_dev_info_ptr(devices_list->list, i);
		const char *authenticator_path = fido_dev_info_path(di);
		fido_dev_t *authenticator = get_device(authenticator_path);
		fido_cbor_info_t *device_info = get_device_info(authenticator);
		if (device_aaguid_matches(cleartext, device_info)) {
			if (!device_supports_hmac_secret(device_info)) {
				free_device_info(device_info);
				continue;
			}

			secret_t *secret = malloc(sizeof(secret_t));
			int result = get_secret_consuming_authenticator_params(
			    authenticator, authenticator_params, secret);
			close_and_free_device_ignoring_errors(authenticator);
			free_device_info(device_info);
			if (result == FIDO_OK) {
				for (int j = 0; j < secret->secret_size; j++) {
					printf("%02x", secret->secret[j]);
				}
				printf("\n");
				return EXIT_SUCCESS;
			}
			warnx("%s at %s did not return a valid secret: %s (0x%x)",
			      fido_dev_info_product_string(di), authenticator_path,
			      fido_strerr(result), result);
		}
	}

	free_cleartext(cleartext);
	cleartext = NULL;

	if (devices_list->count > 0) {
		return EXIT_NO_VALID_AUTHENTICATOR;
	}

	return EXIT_NO_DEVICES;
}

bool device_aaguid_matches(deserialized_cleartext *cleartext,
                           fido_cbor_info_t *device_info) {
	if (cleartext->device_aaguid_size == 0) {
		return true;
	}

	size_t this_device_aaguid_size = fido_cbor_info_aaguid_len(device_info);

	if (this_device_aaguid_size != cleartext->device_aaguid_size) {
		return false;
	}

	unsigned char *this_device_aaguid = malloc(cleartext->device_aaguid_size);
	memcpy(this_device_aaguid, fido_cbor_info_aaguid_ptr(device_info),
	       this_device_aaguid_size);

	for (size_t i = 0; i < cleartext->device_aaguid_size; i++) {
		if (cleartext->device_aaguid[i] != this_device_aaguid[i]) {
			free(this_device_aaguid);
			return false;
		}
	}

	free(this_device_aaguid);
	return true;
}

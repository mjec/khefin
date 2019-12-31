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
	uint16_t device_vendor = cleartext->device_vendor;
	uint16_t device_product = cleartext->device_product;
	bool skip_device_checks = device_vendor == OBFUSCATED_DEVICE_SENTINEL &&
	                          device_product == OBFUSCATED_DEVICE_SENTINEL;

	// Clean up things we don't need anymore
	free_cleartext(cleartext);
	cleartext = NULL;
	sodium_memzero(invocation->passphrase, strlen(invocation->passphrase));
	free(invocation->device);
	free(invocation->file);
	free(invocation->passphrase);
	free(invocation);
	invocation = NULL;

	for (size_t i = 0; i < devices_list->count; i++) {
		const fido_dev_info_t *di = fido_dev_info_ptr(devices_list->list, i);
		if (skip_device_checks ||
		    (fido_dev_info_vendor(di) == device_vendor &&
		     fido_dev_info_product(di) == device_product)) {
			const char *authenticator_path = fido_dev_info_path(di);
			fido_dev_t *authenticator = get_device(authenticator_path);
			secret_t *secret = malloc(sizeof(secret_t));
			int result = get_secret_consuming_authenticator_params(
			    authenticator, authenticator_params, secret);
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
		} else {
			printf("Found device with device_vendor=%x, device_product=%x\n",
			       fido_dev_info_vendor(di), fido_dev_info_product(di));
		}
	}

	if (devices_list->count > 0) {
		free_devices_list(devices_list);
		return EXIT_NO_VALID_AUTHENTICATOR;
	}

	free_devices_list(devices_list);
	return EXIT_NO_DEVICES;
}

#include "enrol.h"

#include <cbor.h>
#include <fido.h>
#include <sodium.h>
#include <string.h>

#include "exit.h"
#include "serialization.h"

void enrol_device(devices_list_t *devices_list,
                  invocation_state_t *invocation) {
	uint16_t device_vendor;
	uint16_t device_product;
	fido_dev_t *authenticator;
	authenticator_parameters_t *authenticator_params;
	deserialized_cleartext *cleartext;

	if (invocation->obfuscate_device_info) {
		device_vendor = OBFUSCATED_DEVICE_SENTINEL;
		device_product = OBFUSCATED_DEVICE_SENTINEL;
	} else {
		bool got_device_info = false;
		for (size_t i = 0; i < devices_list->count; i++) {
			const fido_dev_info_t *di =
			    fido_dev_info_ptr(devices_list->list, i);
			if (strcmp(fido_dev_info_path(di), invocation->device) == 0) {
				device_vendor = fido_dev_info_vendor(di);
				device_product = fido_dev_info_product(di);
				got_device_info = true;
				break;
			}
		}
		if (!got_device_info) {
			errx(EXIT_AUTHENTICATOR_ERROR,
			     "Could not find authenticator in manifest of %d device%s",
			     (int)devices_list->count, devices_list->count == 1 ? "" : "s");
		}
	}

	authenticator = get_device(invocation->device);
	authenticator_params = allocate_parameters(0, SALT_SIZE_BYTES);
	authenticator_params->salt = malloc(SALT_SIZE_BYTES);
	randombytes_buf(authenticator_params->salt, SALT_SIZE_BYTES);

	for (int i = 0; i < RELYING_PARTY_ID_SIZE; i++) {
		authenticator_params->relying_party_id[i] =
		    RPID_ENCODING_TABLE[randombytes_uniform(RPID_ENCODING_TABLE_SIZE)];
	}
	authenticator_params->relying_party_id[RELYING_PARTY_ID_SIZE] = (char)0;
	strcat(authenticator_params->relying_party_id, RELYING_PARTY_SUFFIX);
	create_credential(authenticator, authenticator_params);

	cleartext =
	    build_deserialized_cleartext_from_authenticator_parameters_and_passphrase(
	        authenticator_params, invocation->passphrase);
	cleartext->device_vendor = device_vendor;
	cleartext->device_product = device_product;
	free_parameters(authenticator_params);
	authenticator_params = NULL;
	// We no longer need the passphrase, so zero it out, even though we'll need
	// the rest of invocation later.
	sodium_memzero(invocation->passphrase, strlen(invocation->passphrase));

	write_cleartext_to_file(cleartext, invocation->file);
}

#include "enrol.h"

#include <cbor.h>
#include <fido.h>
#include <sodium.h>
#include <string.h>

#include "exit.h"
#include "files.h"
#include "memory.h"
#include "serialization.h"

void enrol_device(invocation_state_t *invocation) {
	fido_dev_t *authenticator;
	authenticator_parameters_t *authenticator_params;
	deserialized_cleartext *cleartext;

	authenticator = get_device(invocation->device);
	fido_cbor_info_t *device_info = get_device_info(authenticator);
	if (!device_supports_hmac_secret(device_info)) {
		free_device_info(device_info);
		errx(EXIT_AUTHENTICATOR_ERROR,
		     "Device at %s does not support the required hmac-secret extension",
		     invocation->device);
	}

	authenticator_params = allocate_parameters_except_rpid(0, SALT_SIZE_BYTES);
	randombytes_buf(authenticator_params->salt, SALT_SIZE_BYTES);

	authenticator_params->relying_party_id =
	    malloc_or_exit(RELYING_PARTY_ID_SIZE + RELYING_PARTY_SUFFIX_SIZE + 1,
	                   "relying party id in authenticator parameters");
	for (int i = 0; i < RELYING_PARTY_ID_SIZE; i++) {
		authenticator_params->relying_party_id[i] =
		    RPID_ENCODING_TABLE[randombytes_uniform(RPID_ENCODING_TABLE_SIZE)];
	}
	authenticator_params->relying_party_id[RELYING_PARTY_ID_SIZE] = (char)0;
	strncat(authenticator_params->relying_party_id, RELYING_PARTY_SUFFIX,
	        RELYING_PARTY_SUFFIX_SIZE + 1);
	create_credential(authenticator, authenticator_params);
	close_and_free_device_ignoring_errors(authenticator);

	key_spec_t *key_spec = make_new_key_spec_from_invocation(invocation);
	cleartext =
	    build_deserialized_cleartext_from_authenticator_parameters_and_key_spec(
	        authenticator_params, key_spec);
	free_key_spec(key_spec);

	cleartext->device_aaguid_size = fido_cbor_info_aaguid_len(device_info);
	cleartext->device_aaguid =
	    malloc_or_exit(cleartext->device_aaguid_size, "device AAGUID");
	memcpy(cleartext->device_aaguid, fido_cbor_info_aaguid_ptr(device_info),
	       cleartext->device_aaguid_size);
	free_device_info(device_info);
	free_parameters(authenticator_params);
	authenticator_params = NULL;
	// We no longer need the passphrase, so zero it out, even though we'll need
	// the rest of invocation later.
	sodium_memzero(invocation->passphrase, strlen(invocation->passphrase));

	encoded_file *f = write_cleartext(cleartext, invocation->file);
	write_file(f);

	free_cleartext(cleartext);
}

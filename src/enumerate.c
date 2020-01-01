#include "enumerate.h"

#include <fido.h>
#include <stdio.h>

#include "authenticator.h"
#include "exit.h"

void print_devices_list(devices_list_t *devices_list) {
	for (size_t i = 0; i < devices_list->count; i++) {
		const fido_dev_info_t *device_info =
		    fido_dev_info_ptr(devices_list->list, i);
		fido_dev_t *authenticator = get_device(fido_dev_info_path(device_info));

		fido_cbor_info_t *ctap_info = get_device_info(authenticator);

		printf("%s\t", device_supports_hmac_secret(ctap_info) ? " " : "!");

		printf("%s\t", fido_dev_info_path(device_info));

		printf("%s %s\t", fido_dev_info_manufacturer_string(device_info),
		       fido_dev_info_product_string(device_info));

		print_device_aaguid(ctap_info);
		printf("\n");

		free_device_info(ctap_info);
		close_and_free_device_ignoring_errors(authenticator);
	}

	if (devices_list->count == 0) {
		free_devices_list(devices_list);
		errx(EXIT_NO_DEVICES, "No devices found");
	}
}

void print_device_aaguid(fido_cbor_info_t *ctap_info) {
	const unsigned char *device_aaguid = fido_cbor_info_aaguid_ptr(ctap_info);
	size_t device_aaguid_size = fido_cbor_info_aaguid_len(ctap_info);

	for (size_t i = 0; i < device_aaguid_size; i++) {
		printf("%02x", device_aaguid[i]);
		// Add separators for GUID, two characters per byte:
		//  0                       1
		//  0 1 2 3  4 5  6 7  8 9  0 1 2 3 4 5
		// xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
		// NOLINTNEXTLINE(readability-magic-numbers)
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			printf("-");
		}
	}
}

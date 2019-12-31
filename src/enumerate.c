#include "enumerate.h"

#include <fido.h>
#include <stdio.h>

#include "authenticator.h"
#include "exit.h"

void print_devices_list(devices_list_t *devices_list) {
	for (size_t i = 0; i < devices_list->count; i++) {
		const fido_dev_info_t *di = fido_dev_info_ptr(devices_list->list, i);
		printf("%s\t%s %s\n", fido_dev_info_path(di),
		       fido_dev_info_manufacturer_string(di),
		       fido_dev_info_product_string(di));
	}

	if (devices_list->count == 0) {
		free_devices_list(devices_list);
		errx(EXIT_NO_DEVICES, "No devices found");
	}
}

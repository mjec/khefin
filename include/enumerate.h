#ifndef ENUMERATE_H
#define ENUMERATE_H

#include "authenticator.h"

void print_devices_list(devices_list_t *devices_list);
void print_device_aaguid(fido_cbor_info_t *ctap_info);

#endif

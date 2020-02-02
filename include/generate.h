#ifndef GENERATE_H
#define GENERATE_H

#include "authenticator.h"
#include "invocation.h"
#include "serialization_types.h"

unsigned short int
print_secret_consuming_invocation(invocation_state_t *invocation,
                                  devices_list_t *devices_list);
bool device_aaguid_matches(deserialized_cleartext *cleartext,
                           fido_cbor_info_t *device_info);

#endif

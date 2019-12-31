#ifndef fido2hmacsecret_generate_H
#define fido2hmacsecret_generate_H

#include "authenticator.h"
#include "invocation.h"

unsigned short int print_secret(invocation_state_t *invocation,
                                devices_list_t *devices_list);

#endif

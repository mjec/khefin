#ifndef fido2hmacsecret_enrol_H
#define fido2hmacsecret_enrol_H

#include "authenticator.h"
#include "invocation.h"

#define RPID_ENCODING_TABLE_SIZE 32
static const char __attribute__((unused))
RPID_ENCODING_TABLE[RPID_ENCODING_TABLE_SIZE] = {
    /* clang-format off */
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
	'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', '2', '3', '4', '5', '6', '7',
    /* clang-format on */
};

#ifndef SALT_SIZE_BYTES
#define SALT_SIZE_BYTES 64
#endif

void enrol_device(invocation_state_t *invocation);

#endif

#ifndef fido2hmacsecret_exit_H
#define fido2hmacsecret_exit_H

#include <err.h>

// Three categories of error:
//  - user errors, like a bad passphrase
//  - runtime errors, like out of memory
//  - programmer errors, which are bugs
// In each category, we have 5 bits (32 codes) available to be more specific.
// Each category's bitmask MUST be non-zero. Conveniently this means that all
// exit codes less than 0x20 (32) are not specific to this application, as are
// all exit codes greater than or equal to 0x80 (128).
#define _USER_ERROR_BITS 0x20       /* 0b001xxxxx */
#define _RUNTIME_ERROR_BITS 0x40    /* 0b010xxxxx */
#define _PROGRAMMER_ERROR_BITS 0x60 /* 0b011xxxxx */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#define EXIT_BAD_INVOCATION (0 | _USER_ERROR_BITS)
#define EXIT_BAD_PASSPHRASE (1 | _USER_ERROR_BITS)
#define EXIT_NO_DEVICES (2 | _USER_ERROR_BITS)
#define EXIT_NO_VALID_AUTHENTICATOR (3 | _USER_ERROR_BITS)
#define EXIT_DESERIALIZATION_ERROR (4 | _USER_ERROR_BITS)

#define EXIT_OUT_OF_MEMORY (0 | _RUNTIME_ERROR_BITS)
#define EXIT_AUTHENTICATOR_ERROR (1 | _RUNTIME_ERROR_BITS)
#define EXIT_CRYPTOGRAPHY_ERROR (2 | _RUNTIME_ERROR_BITS)
#define EXIT_OVER_PRIVILEGED (3 | _RUNTIME_ERROR_BITS)
#define EXIT_UNABLE_TO_GET_PASSPHRASE (4 | _RUNTIME_ERROR_BITS)

#define EXIT_PROGRAMMER_ERROR (0 | _PROGRAMMER_ERROR_BITS)

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#endif

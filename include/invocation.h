#ifndef INVOCATION_H
#define INVOCATION_H

#include <stdbool.h>
#include <stddef.h>

#ifndef LONGEST_VALID_PASSPHRASE
#define LONGEST_VALID_PASSPHRASE 1024
#endif

// from FIDO2 CTAP spec section 5.6.1:
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html#client-pin-support-requirements
#define LONGEST_VALID_PIN 255

#define NL_CHARACTER_TO_STRIP 0x0a

#define LOWERCASE(x) ((x) | 0x20)
#define UPPERCASE(x) ((x) & ~0x20)

typedef enum subcommand_t {
	subcommand_unknown,
	subcommand_help,
	subcommand_version,
	subcommand_enrol,
	subcommand_generate,
	subcommand_enumerate,
} subcommand_t;

typedef enum kdf_hardness_t {
	kdf_hardness_invalid,
	kdf_hardness_unspecified,
	kdf_hardness_low,
	kdf_hardness_medium,
	kdf_hardness_high,
} kdf_hardness_t;

typedef struct invocation_state_t {
	subcommand_t subcommand;
	char *device;
	char *file;
	char *passphrase;
	char *authenticator_pin;
	bool obfuscate_device_info;
	kdf_hardness_t kdf_hardness;
	char *mixin;
} invocation_state_t;

invocation_state_t *parse_arguments_and_get_passphrase(int argc, char **argv);
void prompt_for_secret(const char *description, size_t maximum_size,
                       char *result);
void free_invocation(invocation_state_t *invocation);

#endif

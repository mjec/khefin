#ifndef fido2hmacsecret_invocation_H
#define fido2hmacsecret_invocation_H

#include <stdbool.h>

#ifndef LONGEST_VALID_PASSPHRASE
#define LONGEST_VALID_PASSPHRASE 1024
#endif

#define NL_CHARACTER_TO_STRIP 0x0a

#define LOWERCASE(x) ((x) | 0x20)

typedef enum subcommand_t {
	unknown,
	help,
	version,
	enrol,
	generate,
	enumerate,
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
	bool obfuscate_device_info;
	kdf_hardness_t kdf_hardness;

} invocation_state_t;

invocation_state_t *parse_arguments_and_get_passphrase(int argc, char **argv);
void free_invocation(invocation_state_t *invocation);

#endif

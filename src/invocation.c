#include "invocation.h"

#include <errno.h>
#include <getopt.h>
#include <sodium.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "exit.h"
#include "files.h"
#include "help.h"
#include "memory.h"

invocation_state_t *parse_arguments_and_get_passphrase(int argc, char **argv) {
	if (argc < 2) {
		print_usage(argv[0]);
		exit(EXIT_BAD_INVOCATION);
	}

	invocation_state_t *result =
	    malloc_or_exit(sizeof(invocation_state_t), "invocation state");
	result->device = NULL;
	result->file = NULL;
	result->passphrase = NULL;
	result->authenticator_pin = NULL;
	result->obfuscate_device_info = false;
	result->kdf_hardness = kdf_hardness_unspecified;
	result->mixin = NULL;

	if (strcmp(argv[1], "help") == 0) {
		result->subcommand = subcommand_help;
	} else if (strcmp(argv[1], "version") == 0) {
		result->subcommand = subcommand_version;
	} else if (strcmp(argv[1], "enrol") == 0) {
		result->subcommand = subcommand_enrol;
	} else if (strcmp(argv[1], "generate") == 0) {
		result->subcommand = subcommand_generate;
	} else if (strcmp(argv[1], "enumerate") == 0) {
		result->subcommand = subcommand_enumerate;
	} else {
		print_usage(argv[0]);
		exit(EXIT_BAD_INVOCATION);
	}

	int c;
	bool invalid_invocation = false;
	while (1) {
		static struct option long_options[] = {
		    {"device", required_argument, 0, 'd'},
		    {"file", required_argument, 0, 'f'},
		    {"passphrase", required_argument, 0, 'p'},
		    {"passphrase-file", required_argument, 0, 'r'},
		    {"pin", required_argument, 0, 'n'},
		    {"mixin", required_argument, 0, 'm'},
		    {"kdf-hardness", required_argument, 0, 'k'},
		    {"obfuscate-device", no_argument, 0, 'o'},
		    {"help", no_argument, 0, 'h'},
		    {NULL, 0, NULL, 0},
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "d:f:p:r:m:k:n:oh", long_options,
		                &option_index);

		if (c == -1) {
			break;
		}

		switch (LOWERCASE(c)) {
		case 'd':
			result->device =
			    strdup_or_exit(optarg, "device path in invocation state");
			break;

		case 'f':
			result->file =
			    strdup_or_exit(optarg, "file path in invocation state");
			break;

		case 'p':
			result->passphrase =
			    strndup_or_exit(optarg, LONGEST_VALID_PASSPHRASE,
			                    "passphrase in invocation state");
			break;

		case 'r': {
			encoded_file *f = read_file(optarg);
			result->passphrase = strndup_or_exit(
			    (const char *)f->data,
			    f->length > LONGEST_VALID_PASSPHRASE ? LONGEST_VALID_PASSPHRASE
			                                         : f->length,
			    "passphrase in invocation state");
			free_encoded_file(f);
		} break;

		case 'n':
			result->authenticator_pin =
			    strndup_or_exit(optarg, LONGEST_VALID_PIN,
			                    "authenticator PIN in invocation state");
			break;

		case 'm':
			result->mixin =
			    strdup_or_exit(optarg, "mixin data in invocation state");
			break;

		case 'k':
			switch (LOWERCASE(optarg[0])) {
			case 'l':
				result->kdf_hardness = kdf_hardness_low;
				break;
			case 'm':
				result->kdf_hardness = kdf_hardness_medium;
				break;
			case 'h':
				result->kdf_hardness = kdf_hardness_high;
				break;
			default:
				result->kdf_hardness = kdf_hardness_invalid;
				break;
			}
			break;

		case 'o':
			result->obfuscate_device_info = true;
			break;

		case 'h':
			result->subcommand = subcommand_help;
			break;

		default:
			invalid_invocation = true;
			break;
		}
	}

	if (result->subcommand == subcommand_unknown && optind < argc) {
		if (strcmp(argv[optind], "help") == 0) {
			result->subcommand = subcommand_help;
		} else if (strcmp(argv[optind], "enrol") == 0) {
			result->subcommand = subcommand_enrol;
		} else if (strcmp(argv[optind], "generate") == 0) {
			result->subcommand = subcommand_generate;
		}
	}

	// Add 1 to optind to take account of the subcommand, if we've already seen
	// it
	optind += (result->subcommand == subcommand_unknown) ? 0 : 1;
	int extra_args = argc - optind;

	if (extra_args > 0) {
		char *program_name = strrchr(argv[0], '/');
		if (program_name == NULL || program_name[1] == (char)0) {
			program_name = argv[0];
		} else {
			program_name += 1;
		}
		fprintf(stderr, "%s: unrecognized argument%s -- ", program_name,
		        extra_args > 1 ? "s" : "");

		for (int i = optind; i < argc; i++) {
			fprintf(stderr, "'%s'%s", argv[i], i < (argc - 1) ? " " : "\n");
		}

		invalid_invocation = true;
	}

	// Required arguments
	switch (result->subcommand) {
	case subcommand_enrol:
		invalid_invocation = invalid_invocation || result->device == NULL ||
		                     result->file == NULL || result->mixin != NULL ||
		                     result->kdf_hardness == kdf_hardness_invalid;
		break;
	case subcommand_generate:
		invalid_invocation = invalid_invocation || result->device != NULL ||
		                     result->file == NULL ||
		                     result->obfuscate_device_info ||
		                     result->kdf_hardness != kdf_hardness_unspecified;
		break;
	case subcommand_enumerate:
	case subcommand_help:
	case subcommand_version:
	default:
		invalid_invocation = invalid_invocation || result->device != NULL ||
		                     result->file != NULL || result->mixin != NULL ||
		                     result->passphrase != NULL ||
		                     result->obfuscate_device_info ||
		                     result->kdf_hardness != kdf_hardness_unspecified;
		break;
	}

	if (result->subcommand != subcommand_help && invalid_invocation) {
		free_invocation(result);
		print_usage(argv[0]);
		exit(EXIT_BAD_INVOCATION);
	}

	if (result->subcommand == subcommand_enrol &&
	    result->kdf_hardness == kdf_hardness_unspecified) {
		long pages = sysconf(_SC_PHYS_PAGES);
		long page_size = sysconf(_SC_PAGE_SIZE);
		size_t available_memory = pages * page_size;
		if (available_memory > (crypto_pwhash_MEMLIMIT_SENSITIVE * 2)) {
			result->kdf_hardness = kdf_hardness_high;
		} else if (available_memory > (crypto_pwhash_MEMLIMIT_MODERATE * 2)) {
			result->kdf_hardness = kdf_hardness_medium;
		} else {
			result->kdf_hardness = kdf_hardness_low;
		}
	}

	if (result->subcommand == subcommand_enrol ||
	    result->subcommand == subcommand_generate) {
		if (result->passphrase == NULL) {
			result->passphrase =
			    malloc_or_exit(LONGEST_VALID_PASSPHRASE + 1, "passphrase");
			prompt_for_secret("passphrase", LONGEST_VALID_PASSPHRASE,
			                  result->passphrase);
		}
	}

	return result;
}

void prompt_for_secret(const char *description, size_t maximum_size,
                       char *result) {
	if (isatty(STDIN_FILENO)) {
		struct termios terminal_settings;
		if (tcgetattr(STDIN_FILENO, &terminal_settings) != 0) {
			err(EXIT_UNABLE_TO_GET_USER_SECRET, "Unable to get %s",
			    description);
		}
		tcflag_t previous_c_lflag = terminal_settings.c_lflag;
		terminal_settings.c_lflag &= ~(ECHO | ECHOE | ECHOK);
		terminal_settings.c_lflag |= ECHONL;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal_settings) != 0) {
			err(EXIT_UNABLE_TO_GET_USER_SECRET, "Unable to get %s",
			    description);
		}
		fprintf(stderr, "%c%s: ", UPPERCASE(description[0]), description + 1);
		if (fgets(result, maximum_size, stdin) == NULL) {
			errx(EXIT_UNABLE_TO_GET_USER_SECRET,
			     "Unable to get %s on STDIN: fgets error 0x%02x", description,
			     ferror(stdin));
		}

		// Remove trailing \n
		if (strlen(result) > 0 &&
		    result[strlen(result) - 1] == NL_CHARACTER_TO_STRIP) {
			result[strlen(result) - 1] = 0x00;
		}

		// Reset settings
		terminal_settings.c_lflag = previous_c_lflag;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal_settings) != 0) {
			errx(EXIT_UNABLE_TO_GET_USER_SECRET,
			     "Unable to reset terminal after getting %s", description);
		}

	} else if (errno == ENOTTY) {
		if (fgets(result, maximum_size, stdin) == NULL) {
			errx(EXIT_UNABLE_TO_GET_USER_SECRET,
			     "Unable to get %s on STDIN: fgets error 0x%02x", description,
			     ferror(stdin));
		}

		// Remove trailing \n
		if (result[strlen(result) - 1] == NL_CHARACTER_TO_STRIP) {
			result[strlen(result) - 1] = 0x00;
		}

	} else {
		err(EXIT_UNABLE_TO_GET_USER_SECRET, "Unable to get %s", description);
	}
}

void free_invocation(invocation_state_t *invocation) {
	if (invocation == NULL) {
		return;
	}

	if (invocation->passphrase != NULL) {
		sodium_memzero(invocation->passphrase, strlen(invocation->passphrase));
		free(invocation->passphrase);
	}

	if (invocation->mixin != NULL) {
		free(invocation->mixin);
	}

	if (invocation->device != NULL) {
		free(invocation->device);
	}

	if (invocation->file != NULL) {
		free(invocation->file);
	}

	free(invocation);
}

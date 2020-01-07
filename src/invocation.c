#include "invocation.h"

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "exit.h"
#include "help.h"

invocation_state_t *parse_arguments_and_get_passphrase(int argc, char **argv) {
	if (argc < 2) {
		print_usage(argv[0]);
		exit(EXIT_BAD_INVOCATION);
	}

	invocation_state_t *result = malloc(sizeof(invocation_state_t));
	if (result == NULL) {
		err(EXIT_OUT_OF_MEMORY,
		    "Unable to allocate memory for invocation state");
	}
	result->device = NULL;
	result->file = NULL;
	result->passphrase = NULL;
	result->obfuscate_device_info = false;

	if (strcmp(argv[1], "help") == 0) {
		result->subcommand = help;
	} else if (strcmp(argv[1], "version") == 0) {
		result->subcommand = version;
	} else if (strcmp(argv[1], "enrol") == 0) {
		result->subcommand = enrol;
	} else if (strcmp(argv[1], "generate") == 0) {
		result->subcommand = generate;
	} else if (strcmp(argv[1], "enumerate") == 0) {
		result->subcommand = enumerate;
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
		    {"obfuscate-device", no_argument, 0, 'o'},
		    {"help", no_argument, 0, 'h'},
		    {NULL, 0, NULL, 0},
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "d:f:p:oh", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'd':
			result->device = malloc(strlen(optarg) + 1);
			strcpy(result->device, optarg);
			break;

		case 'f':
			result->file = malloc(strlen(optarg) + 1);
			strcpy(result->file, optarg);
			break;

		case 'p':
			result->passphrase = malloc(strlen(optarg) + 1);
			strcpy(result->passphrase, optarg);
			break;

		case 'o':
			result->obfuscate_device_info = true;
			break;

		case 'h':
			result->subcommand = help;
			break;

		default:
			invalid_invocation = true;
			break;
		}
	}

	if (result->subcommand == unknown && optind < argc) {
		if (strcmp(argv[optind], "help") == 0) {
			result->subcommand = help;
		} else if (strcmp(argv[optind], "enrol") == 0) {
			result->subcommand = enrol;
		} else if (strcmp(argv[optind], "generate") == 0) {
			result->subcommand = generate;
		}
	}

	// Add 1 to optind to take account of the subcommand, if we've already seen
	// it
	optind += (result->subcommand == unknown) ? 0 : 1;
	int extra_args = argc - optind;

	if (extra_args > 0) {
		size_t warning_size = strlen("invalid arguments -- \n") + 1;
		for (int i = optind; i < argc; i++) {
			warning_size += strlen(argv[i]) + 3;
		}
		char *error_message = malloc(warning_size * (sizeof(char)));

		strcat(error_message, "invalid argument");
		if (extra_args > 1) {
			strcat(error_message, "s");
		}
		strcat(error_message, " -- ");

		while (optind < argc) {
			strcat(error_message, "'");
			strcat(error_message, argv[optind++]);
			strcat(error_message, "'");
			if (optind != argc) {
				strcat(error_message, " ");
			}
		}
		warnx("%s", error_message);
		free(error_message);
		invalid_invocation = true;
	}

	// Required arguments
	switch (result->subcommand) {
	case enrol:
		invalid_invocation = invalid_invocation || result->device == NULL ||
		                     result->file == NULL;
		break;
	case generate:
		invalid_invocation = invalid_invocation || result->device != NULL ||
		                     result->file == NULL ||
		                     result->obfuscate_device_info;
		break;
	case enumerate:
		invalid_invocation = invalid_invocation || result->device != NULL ||
		                     result->file != NULL ||
		                     result->passphrase != NULL ||
		                     result->obfuscate_device_info;
		break;
	case help:
	case version:
	default:
		invalid_invocation =
		    invalid_invocation || result->obfuscate_device_info;
		break;
	}

	if (result->subcommand != help && invalid_invocation) {
		print_usage(argv[0]);
		exit(EXIT_BAD_INVOCATION);
	}

	if (result->passphrase == NULL &&
	    (result->subcommand == enrol || result->subcommand == generate)) {
		result->passphrase = malloc(LONGEST_VALID_PASSPHRASE + 1);
		if (result->passphrase == NULL) {
			err(EXIT_OUT_OF_MEMORY, "Unable to allocate memory for passphrase");
		}

		if (isatty(STDIN_FILENO)) {
			struct termios terminal_settings;
			if (tcgetattr(STDIN_FILENO, &terminal_settings) != 0) {
				err(EXIT_UNABLE_TO_GET_PASSPHRASE, "Unable to get passphrase");
			}
			tcflag_t previous_c_lflag = terminal_settings.c_lflag;
			terminal_settings.c_lflag &= ~(ECHO | ECHOE | ECHOK);
			terminal_settings.c_lflag |= ECHONL;
			if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal_settings) != 0) {
				err(EXIT_UNABLE_TO_GET_PASSPHRASE, "Unable to get passphrase");
			}
			printf("Passphrase: ");
			if (fgets(result->passphrase, LONGEST_VALID_PASSPHRASE, stdin) ==
			    NULL) {
				errx(EXIT_UNABLE_TO_GET_PASSPHRASE,
				     "Unable to get passphrase on STDIN: fgets error 0x%02x",
				     ferror(stdin));
			}

			// Remove trailing \n
			if (result->passphrase[strlen(result->passphrase) - 1] ==
			    NL_CHARACTER_TO_STRIP) {
				result->passphrase[strlen(result->passphrase) - 1] = 0x00;
			}

			// Reset settings
			terminal_settings.c_lflag = previous_c_lflag;
			if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal_settings) != 0) {
				err(EXIT_UNABLE_TO_GET_PASSPHRASE,
				    "Unable to reset terminal after getting passphrase");
			}
		} else if (errno == ENOTTY) {
			if (fgets(result->passphrase, LONGEST_VALID_PASSPHRASE, stdin) ==
			    NULL) {
				errx(EXIT_UNABLE_TO_GET_PASSPHRASE,
				     "Unable to get passphrase on STDIN: fgets error 0x%02x",
				     ferror(stdin));
			}
		} else {
			err(EXIT_UNABLE_TO_GET_PASSPHRASE, "Unable to get passphrase");
		}
	}

	return result;
}

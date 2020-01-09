#include <fido.h>
#include <sodium.h>

#include "authenticator.h"
#include "enrol.h"
#include "enumerate.h"
#include "exit.h"
#include "generate.h"
#include "help.h"
#include "invocation.h"
#include "memory.h"

int main(int argc, char **argv) {
	lock_memory_and_drop_privileges();

	unsigned short int print_secret_result;

	if (sodium_init() != 0) {
		errx(EXIT_CRYPTOGRAPHY_ERROR, "Unable to initialize libsodium");
	}

	fido_init(0);

	invocation_state_t *invocation =
	    parse_arguments_and_get_passphrase(argc, argv);

	devices_list_t *devices_list;

	switch (invocation->subcommand) {
	case help:
		print_help(argv[0]);
		free_invocation(invocation);
		return EXIT_SUCCESS;

	case version:
		print_version();
		free_invocation(invocation);
		return EXIT_SUCCESS;

	case enumerate:
		devices_list = list_devices();
		print_devices_list(devices_list);
		free_devices_list(devices_list);
		free_invocation(invocation);
		return EXIT_SUCCESS;

	case enrol:
		enrol_device(invocation);
		free_invocation(invocation);
		return EXIT_SUCCESS;

	case generate:
		devices_list = list_devices();
		print_secret_result =
		    print_secret_consuming_invocation(invocation, devices_list);
		free_devices_list(devices_list);
		switch (print_secret_result) {
		case EXIT_NO_DEVICES:
			errx(EXIT_NO_DEVICES,
			     "Unable to find an appropriate authenticator to generate a "
			     "secret");
		case EXIT_NO_VALID_AUTHENTICATOR:
			errx(EXIT_NO_VALID_AUTHENTICATOR,
			     "No connected authenticator was able to generate a valid "
			     "secret");
		case EXIT_SUCCESS:
			return EXIT_SUCCESS;
		default:
			errx(EXIT_PROGRAMMER_ERROR,
			     "BUG (%s:%d): unhandled return value from "
			     "print_secret_consuming_invocation() (%d)",
			     __func__, __LINE__, print_secret_result);
		}
		break;

	default:
		errx(EXIT_PROGRAMMER_ERROR,
		     "BUG (%s:%d): unhandled but valid subcommand (%d)\n", __func__,
		     __LINE__, invocation->subcommand);
	}
}

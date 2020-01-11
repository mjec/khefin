#include "help.h"

#include <stdio.h>
#include <string.h>

void print_version(void) { printf("%s %s\n", APPNAME, APPVERSION); }

void print_usage(char *program_name) {
	char *program_basename = strrchr(program_name, '/');
	if (program_basename != NULL && program_basename[1] != (char)0) {
		program_name = program_basename + 1;
	}
	printf("Usage: %s help\n"
	       "       %s version\n"
	       "       %s enumerate\n"
	       "       %s generate -f <file> [-p <passphrase>] [-m <data>]\n"
	       "       %s enrol -d <device> -f <file> [-p <passphrase>] [-o]\n"
	       "       %*s      [-k <hardness>]\n",
	       program_name, program_name, program_name, program_name, program_name,
	       (int)strlen(program_name), "  ");
}

void print_help(char *program_name) {
	print_usage(program_name);
	printf(
	    "%s",
	    /* clang-format off */
	    // This has manually-inserted hard wraps at 80 characters,
	    // which is inconsistent with clang-format.
	    "\n"
	    "help       print this screen and exit.\n"
	    "\n"
	    "version    print version and exit.\n"
	    "\n"
	    "enumerate  show a list of authenticator devices currently connected.\n"
	    "\n"
	    "enrol       create or overwrite <file> with randomly-generated data required\n"
	    "            to produce a secret for the given passphrase.\n"
	    "\n"
	    "generate   generate an HMAC across the data contained in <file>, once it has\n"
	    "           been decrypted with the given passphrase.\n"
	    "\n"
	    "   -d, --device <device>           REQUIRED for enrol. The path to the\n"
	    "                                   authenticator to enrol, e.g. /dev/hidraw0.\n"
	    "                                   This device MUST support the "
	    "FIDO2\n"
	    "                                   hmac-secret extension (supported by most \n"
	    "                                   Yubico Security Keys and YubiKeys).\n"
	    "\n"
	    "   -f, --file <file>               REQUIRED for enrol or generate. The file to\n"
	    "                                   write to (for enrol; this file will be\n"
	    "                                   overwritten), or read from (for generate).\n"
	    "\n"
	    "   -p, --passphrase <passphrase>   The passphrase to use. If not specified,\n"
	    "                                   you will be prompted for a passphrase.\n"
	    "\n"
	    "   -o, --obfuscate-device-info     If specified for enrol, do not store the.\n"
	    "                                   device vendor and product ID in <file>.\n"
	    "\n"
	    "   -k, --kdf-hardness <hardness>   Specify the complexity of the key derivation\n"
	    "                                   function used to derive a cryptographic key\n"
	    "                                   from <passphrase>. Valid options are high,\n"
	    "                                   medium or low. If not specified, a value\n"
	    "                                   will be chosen automatically based on\n"
	    "                                   total system RAM.\n"
		"\n"
		"   -m, --mixin <data>              Combine <data> with the encrypted salt,\n"
		"                                   so that the returned value depends on it.\n"
		"                                   Note that setting <data> to an empty\n"
		"                                   string behaves differently to not using\n"
		"                                   this argument at all.\n"
	    "\n"
	    "The output of this program on STDOUT (in either enrol or generate mode) will be\n"
	    "a sequence of printable, URL-safe ASCII characters, that depend on the\n"
	    "randomly generated parameters placed in the file, the authenticator device and\n"
	    "the passphrase. This will be followed by a single newline.\n"
	    /* clang-format on */
	);
}

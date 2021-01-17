#include "memory.h"

#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>

#include "exit.h"

void lock_memory_and_drop_privileges(void) {
	int r;

	// Prevent our memory from being swapped out
	r = mlockall(MCL_CURRENT | MCL_FUTURE);
#if WARN_ON_MEMORY_LOCK_ERRORS
	if (r != 0) {
		warn("Unable to lock memory, which means secrets may be swapped to "
		     "disk");
	}
#endif

#ifndef DEBUG

	// Set memory to not-dumpable
	r = prctl(PR_SET_DUMPABLE, 0);
#if WARN_ON_MEMORY_LOCK_ERRORS
	if (r != 0) {
		warn("Unable to set dumpable flag to 0, which means a core dump "
		     "(including  secrets) may be written to disk in the event of a "
		     "crash");
	}
#endif

	// Limit core dump size to 0 bytes
	r = setrlimit(RLIMIT_CORE, &(struct rlimit){0, 0});
#if WARN_ON_MEMORY_LOCK_ERRORS
	if (r != 0) {
		warn("Unable to set RLIMIT_CORE to {0, 0}, which means a core dump "
		     "(including secrets) may be written to disk in the event of a "
		     "crash");
	}
#endif

#endif // ifndef DEBUG
}

void *malloc_or_exit(size_t n, const char *what) {
	void *result = malloc(n);
	if (result == NULL) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to allocate memory for %s", what);
	}
	return result;
}

char *strdup_or_exit(const char *str, const char *what) {
	char *result = strdup(str);
	if (result == NULL) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to allocate memory for %s", what);
	}
	return result;
}

char *strndup_or_exit(const char *str, size_t n, const char *what) {
	char *result = strndup(str, n);
	if (result == NULL) {
		errx(EXIT_OUT_OF_MEMORY, "Unable to allocate memory for %s", what);
	}
	return result;
}

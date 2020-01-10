#include "memory.h"

#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>

#include "exit.h"

void lock_memory_and_drop_privileges(void) {
	int r;
	int original_uid;

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

	// Drop privileges if running as root
	if (geteuid() == 0) {
		if (geteuid() != getuid()) {
			original_uid = getuid();
		} else if (getenv("SUDO_UID") != NULL) {
			original_uid = atoi(getenv("SUDO_UID"));
			if (original_uid < 1) {
				original_uid = 0;
			}
		} else {
			original_uid = 0;
		}

		if (original_uid != 0) {
			r = setrlimit(
			    RLIMIT_MEMLOCK,
			    &(struct rlimit){MEMLOCK_LIMIT_BYTES, MEMLOCK_LIMIT_BYTES});
			if (r != 0) {
#if WARN_ON_MEMORY_LOCK_ERRORS
				warn("Unable to set RLIMIT_MEMLOCK soft and hard limits to %d "
				     "bytes, which may cause out of memory errors. Disabling "
				     "memory locking for future allocations.",
				     MEMLOCK_LIMIT_BYTES);
#endif
				munlockall();
				mlockall(MCL_CURRENT);
			}
			if (setuid(original_uid) != 0) {
				err(EXIT_OVER_PRIVILEGED, "Unable to drop privileges");
			}
		}
	}
}

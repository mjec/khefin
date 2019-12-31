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
	bool running_as_root = (geteuid() == 0);
	bool ignore_memory_locking_errors =
	    ALWAYS_SILENCE_MEMORY_LOCK_ERRORS ||
	    (getenv(SILENCE_MEMORY_LOCK_ERRORS_ENV_VAR) != NULL);

	if (!running_as_root && !ignore_memory_locking_errors) {
		warnx("Not running as setuid root (running as UID %d), which may leave "
		      "memory open to swapping out",
		      geteuid());
	}

	// Prevent our memory from being swapped out, if we can
	int mlockall_return = mlockall(MCL_CURRENT | MCL_FUTURE);
	if (mlockall_return != 0 && !ignore_memory_locking_errors) {
		warn("Unable to lock memory, which means secrets may be swapped to "
		     "disk");
	}

	if (prctl(PR_SET_DUMPABLE, 0) != 0 && !ignore_memory_locking_errors) {
		warn("Unable to set dumpable flag to 0, which means a core dump "
		     "(including  secrets) may be written to disk in the event of a "
		     "crash");
	}

	if (setrlimit(RLIMIT_CORE, &(struct rlimit){0, 0}) != 0 &&
	    !ignore_memory_locking_errors) {
		warn("Unable to set RLIMIT_CORE to {0, 0}, which means a core dump "
		     "(including secrets) may be written to disk in the event of a "
		     "crash");
	}

	if (setrlimit(RLIMIT_MEMLOCK, &(struct rlimit){MEMLOCK_LIMIT_BYTES,
	                                               MEMLOCK_LIMIT_BYTES}) != 0) {
		if (!ignore_memory_locking_errors) {
			warn(
			    "Unable to set RLIMIT_MEMLOCK soft and hard limits to %d "
			    "bytes, which may cause out of memory errors. Disabling memory "
			    "locking for future allocations.",
			    MEMLOCK_LIMIT_BYTES);
		}
		munlockall();
		mlockall(MCL_CURRENT);
	}

	if (running_as_root && getuid() != geteuid()) {
		if (setuid(getuid()) != 0) {
			err(EXIT_OVER_PRIVILEGED, "Unable to drop privileges");
		}
	}
}

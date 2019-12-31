#ifndef fido2hmacsecret_privilege_H
#define fido2hmacsecret_privilege_H

#ifndef ALWAYS_SILENCE_MEMORY_LOCK_ERRORS
#define ALWAYS_SILENCE_MEMORY_LOCK_ERRORS 0
#endif

#define SILENCE_MEMORY_LOCK_ERRORS_ENV_VAR                                     \
	"FIDO2_HMAC_SECRET_SILENCE_MEMLOCK_ERRORS"

#ifndef MEMLOCK_LIMIT_BYTES
// 512MiB ought to be enough for anyone
#define MEMLOCK_LIMIT_BYTES (512 * 1024 * 1024)
#endif

void lock_memory_and_drop_privileges(void);

#endif

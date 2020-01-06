#ifndef fido2hmacsecret_privilege_H
#define fido2hmacsecret_privilege_H

#ifndef WARN_ON_MEMORY_LOCK_ERRORS
#define WARN_ON_MEMORY_LOCK_ERRORS 1
#endif

// We need 256MB for the key derivation function.
// Add 64MB for everything else we could need.
#define MEMLOCK_LIMIT_BYTES 335544320

void lock_memory_and_drop_privileges(void);

#endif

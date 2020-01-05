#ifndef fido2hmacsecret_privilege_H
#define fido2hmacsecret_privilege_H

#ifndef WARN_ON_MEMORY_LOCK_ERRORS
#define WARN_ON_MEMORY_LOCK_ERRORS 1
#endif

void lock_memory_and_drop_privileges(void);

#endif

## Next version (unreleased)
_These changes are on the branch `master`, but not yet in a versioned release._

* Refactor code to make it possible to compile with GCC without warnings
* Update CI to run make against gcc as well as clang

## Version 0.4.0

* Make passphrase prompt output on STDERR instead of STDOUT
* Add fido2-hmac-secret-add-luks-key script (issue #1)
* Add setcap cap_ipc_lock+ep in install target, SETCAP_BINARY install flag (defaults to on), and remove setuid (issue #3)
* Change ALWAYS_SILENCE_MEMORY_LOCK_ERRORS to WARN_ON_MEMORY_LOCK_ERRORS and have it default to on
* Remove FIDO2_HMAC_SECRET_SILENCE_MEMLOCK_ERRORS environment variable
* Move bash-completion script from docs/ to scripts/
* Do not generate bash-completion script by default
* Include reasons for setuid root in README, and add ALWAYS_SILENCE_MEMORY_LOCK_ERRORS compile option (issue #3)
* Permit passphrase to be supplied on STDIN, if not a tty (issue #2)

## Version 0.3.0

* Add mkinitcpio hooks for using this with encrypted root device on Arch Linux
* Change "password" to "passphrase" everywhere
* Change `get_secret_consuming_authenticator_params` to stop consuming params, thus avoiding double-free
* Fix bug where providing an invalid subcommand did not print usage and exit with EXIT_BAD_INVOCATION
* Improve bash completion, removing invalid options for `generate`

## Version 0.2.0

* Remove use of device vendor & product ID, replace with AAGUID
* Fix double free bugs when the wrong authenticator device is used
* Fix reference to algorithm as uint32 in serialzie.c
* Add escaping for `-` in manpage.m4
* Convert spaces to tabs in bash-completion.m4

## Version 0.1.0

* Initial release.

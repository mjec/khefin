## Next version (unreleased)
_These changes are on the branch `master`, but not yet in a versioned release._

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
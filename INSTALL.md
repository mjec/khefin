# Installation

## Dependencies

Before building or running this tool, you'll need the following dependencies installed:

 * [libfido2](https://developers.yubico.com/libfido2/)
 * [libcbor](https://libcbor.readthedocs.io/en/v0.5.0/) - also a dependency of libfido2
 * [libsodium](https://download.libsodium.org/doc/)
 * [libcap](https://sites.google.com/site/fullycapable/)

At the moment I believe this tool is linux-only; issue reports or pull requests to improve portability are gratefully accepted.

## Building

`make release` is the main compile target.

If you want a [bash-completion](https://github.com/scop/bash-completion) script, add the `bash-completion` target.

If you want `mkinitcpio` hooks add the `initcpio` target.

### Build flags

| Flag | Type | Default | Meaning |
|------|------|---------|---------|
| `LONGEST_VALID_PASSPHRASE` | integer | 1024 | length after which passphrases are truncated |
| `WARN_ON_MEMORY_LOCK_ERRORS` | boolean | 1 | if `0`, this will disable warnings when memory cannot be locked |
| `SETCAP_BINARY` | boolean | 1 | if `0`, then `make install` will not attempt to add `CAP_IPC_LOCK` to the installed binary |
| `DEBUG` | N/A | unset | if set to any value, this will enable debug mode (enabling core dumps, assertions and symbols) |

Standard makefile variables (`CC`, `CFLAGS`, `LDFLAGS`, `DESTDIR`, `PREFIX`) are respected.

You can set the maximum passphrase length (defaults to 1024; everything after this will be ignored) by appending `LONGEST_VALID_PASSPHRASE=<some integer>` after the list of targets.

You disable memory locking warnings (default to on) by setting `WARN_ON_MEMORY_LOCK_ERRORS=0` (see the memory locking section below).

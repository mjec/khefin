# Installation

## Dependencies

Before building or running this tool, you'll need the following dependencies installed:

* [libfido2](https://developers.yubico.com/libfido2/) and its dependency libcrypto from OpenSSL
* [libcbor](https://libcbor.readthedocs.io/en/v0.5.0/) - also a dependency of libfido2
* [libsodium](https://download.libsodium.org/doc/)

At the moment I believe this tool is linux-only; issue reports or pull requests to improve portability are gratefully accepted.

To build this tool, you'll also need:

* [clang](https://clang.llvm.org/) or [gcc](https://gcc.gnu.org/)
* [m4](https://www.gnu.org/software/m4/m4.html)
* [make](https://www.gnu.org/software/make/)
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)

## Building

`make release` is the main compile target.

If you want a [bash-completion](https://github.com/scop/bash-completion) script, add the `bash-completion` target.

Run `make help` for a list of other targets.

### Build flags

| Flag | Type | Default | Meaning |
|------|------|---------|---------|
| `LONGEST_VALID_PASSPHRASE` | integer | 1024 | length after which passphrases are truncated |
| `WARN_ON_MEMORY_LOCK_ERRORS` | boolean | 1 | if `0`, this will disable warnings when memory cannot be locked |
| `SETCAP_BINARY` | boolean | 1 | if `0`, then `make install` will not attempt to add `CAP_IPC_LOCK` to the installed binary |
| `DEBUG` | N/A | unset | if set to any value, this will enable debug mode (enabling core dumps, assertions and symbols) |

Standard makefile variables (`CC`, `CFLAGS`, `LDFLAGS`, `DESTDIR`, `PREFIX`) are respected.

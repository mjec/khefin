# fido2-hmac-secret
A system for using a FIDO2 authenticator with [hmac-secret extension](https://fidoalliance.org/specs/fido-v2.0-id-20180227/fido-client-to-authenticator-protocol-v2.0-id-20180227.html#sctn-hmac-secret-extension) support to generate passphrase-protected secrets.

## Installation

### Dependencies

Before building or running this tool, you'll need the following dependencies installed:

 * [libfido2](https://developers.yubico.com/libfido2/)
 * [libcbor](https://libcbor.readthedocs.io/en/v0.5.0/) - also a dependency of libfido2
 * [libsodium](https://download.libsodium.org/doc/)
 * [libcap](https://sites.google.com/site/fullycapable/) - required for `make install` by default/with `SETCAP_BINARY=1`

At the moment I believe this tool is linux-only; issue reports or pull requests to improve portability are gratefully accepted.

### Building

`make release` is the main compile target.

If you want a [bash-completion](https://github.com/scop/bash-completion) script, add the `bash-completion` target; and if you want `mkinitcpio` hooks add the `initcpio` target. To compile with all, run `make release bash-completion initcpio`.

You can set the maximum passphrase length (defaults to 1024; everything after this will be ignored) by appending `LONGEST_VALID_PASSPHRASE=<some integer>` after the list of targets.

You disable memory locking warnings (default to on) by setting `WARN_ON_MEMORY_LOCK_ERRORS=0` (see the memory locking section below).

### Installing

`make install` is the main installation target. That will install bash completions and `mkinitcpio` hooks if they were compiled.

The `DESTDIR` and `PREFIX` variables are respected (so you can do `sudo make install PREFIX=/usr`, for example, to install to `/usr/bin` instead of `/usr/local/bin`).

By default `make install` will give the installed binary `CAP_IPC_LOCK` capabilities (see the memory locking section below). You can skip this by running `make install SETCAP_BINARY=0` instead.

## Memory locking

To avoid secrets accidentally being written to disk (including swap space), the binary will disable core dumps and lock all allocated memory. The success of this depends on your locally-configured limits (and in particular `RLIMIT_MEMLOCK`) and the capabilities of the binary. To ensure success, by default the binary is given the `CAP_IPC_LOCK` capability during `make install` (see above). This bypasses `RLIMIT_MEMLOCK`.

If for any reason core dumps cannot be disabled or memory cannot be locked, a warning will be generated, unless the binary was compiled with `WARN_ON_MEMORY_LOCK_ERRORS=0`.

## How this works

The authenticator has a credential-scoped secret which is used to calculate the HMAC-SHA256 over some data they call a salt.

During the `enrol` step, we create a file containing a randomly-generated salt, a credential ID (specified by the authenticator), and a randomly generated relying party ID (a credential on the authenticator is scoped to a relying party ID, which is specified when creating the credential). This data is encrypted with the passphrase you specify, and saved to disk.

The `generate` command decrypts this data, sends it to the authenticator, and prints the result. This results in the same value being returned every time; but that value cannot be obtained without both the decrypted keyfile (which requires your passphrase) and the authenticator device.

### Encrypted keyfile

This is a CBOR-encoded array with the following elements:

| Field | Name            | Type                    | Notes                         |
|:-----:|-----------------|-------------------------|-------------------------------|
| 0     | version         | unsigned 8 bit integer  | Schama version; always `1`    |
| 1     | device AAGUID   | definite bytestring     | Device make & model, or empty |
| 2     | passphrase salt | definite bytestring     | See `crypto_pwhash`           |
| 3     | opslimit        | unsigned 64 bit integer | See `crypto_pwhash`           |
| 4     | memlimit        | unsigned 64 bit integer | See `crypto_pwhash`           |
| 5     | algorithm       | unsigned 16 bit integer | See `crypto_pwhash`           |
| 6     | nonce           | definite bytestring     | See `crypto_secretbox_easy`   |
| 7     | encrypted data  | definite bytestring     |                               |

Device AAGUID will be empty if and only if the `enrol` step is done with `--obfuscate-device-info`. If it's empty, every hmac-secret-supporting device will be tried during the `generate` step. If it's not empty, only devices with a matching AAGUID are returned.

Any modification of any of the fields (except version, device vendor and device product) will irrecoverably render the key unusable.

The user will be prompted for a passphrase, which is run through libsodium's `crypto_pwhash`, with parameters from fields 3, 4, 5 and 6 to give a key.

That key, combined with the nonce in field 7, is used to decrypt the encrypted data in field 8 with libsodium's `crypto_secretbox_easy`.

Once the encrypted data section is decrypted, it contains a CBOR-encoded array with the following elements:

| Field | Name             | Type                   | Notes                                                 |
|:-----:|------------------|------------------------|-------------------------------------------------------|
| 0     | version          | unsigned 8 bit integer | Schama version; always `1`                            |
| 1     | relying party ID | definite UTF-8 string  | random subdomain of `.v1.fido2-hmac-secret.localhost` |
| 2     | credential ID    | definite bytestring    |                                                       |
| 3     | HMAC salt        | definite bytestring    |                                                       |

These parameters (other than version) are then passed to the FIDO2 authenticator, which returns an [HMAC-SHA-256 over the salt](https://fidoalliance.org/specs/fido-v2.0-id-20180227/fido-client-to-authenticator-protocol-v2.0-id-20180227.html#sctn-hmac-secret-extension). The key for that HMAC is available only to the authenticator, and is associated with the credential ID and relying party ID. This means that all three fields are essential, as is they physical authenticator device.

In fact it would be sufficient for cryptographic security of the key material to store hide the HMAC salt, which provides at least 32 bytes (and normally 64 bytes or 512 bits), of entropy. Hiding the relying party ID and credential ID however costs us nothing, and adds some additional protection.

At the very least, the credential ID offers an additional [100 bits of entropy](https://www.w3.org/TR/webauthn/#credential-id). Although the credential ID is opaque, it may contain the key material enrypted in a manner that only the FIDO2 authenticator can decrypt. In this case, even advanced tampering with that device would not reveal enough information to even begin an offline attack, absent the cleartext credential ID.

The relying party ID contained in this data is in fact only used as part of that ID, and it is always a 32 character string composed of characters in the range [a-z0-7], for a total of 160 bits of entropy. The aim here is to ensure that any protections in the authenticator against cross-origin key use detection are available. I doubt any key has such protection, but again it costs us nothing.

## Risks

The number one risk is that by playing around with encryption like this you will lose your data. Keep good, offline backups of your data. Test your backups regularly, to ensure files can be recovered without access to any of your usual hardware or software.

[Backup your LUKS header](https://gitlab.com/cryptsetup/cryptsetup/wikis/FrequentlyAskedQuestions#6-backup-and-data-recovery) and data before using this for disk encryption keys. Seriously.

Keep a backup of the keyfile created by this application as well. If you lose that file, it is impossible to recover the secret.

The security of this system depends on the security of your authenticator device, libsodium, libfido2, and the quality of your passphrase. It's also possible -- and indeed more likely than any of the former issues -- that there's a bug in the code for this application which compromises its security somehow. Pull requests and issues are very welcome.

## Warrant canary (but not a warranty)

At the time of writing, I have not received or complied with any government or non-government requests for information or services relating to this software.

## Output format

The usual output format is a series of 128 characters ASCII, being the hex-encoded (lowercase) form of the key, with no whitespace (plus a new line character). This avoids potential problems with interfaces that are not 8-bit clean (including the risk of a NUL byte or newline causing key material truncation). You can also type in such a key by hand with any keyboard, if that is important.


# Internals

## How this works

The authenticator has a credential-scoped secret which is used to calculate the HMAC-SHA256 over some data (the "salt").

During the `enrol` step, we create a file ("encrypted keyfile") containing a randomly-generated salt, a credential ID (specified by the authenticator), and a randomly generated relying party ID (a credential on the authenticator is scoped to a relying party ID, which is specified when creating the credential). This data is encrypted with the passphrase you specify (using libsodium), and saved to disk.

The `generate` command decrypts this data, sends it to the authenticator, and prints the result. This results in the same value being returned every time; but that value cannot be obtained without both the decrypted keyfile (which requires your passphrase) and the authenticator device.

## Encrypted keyfile

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


## Memory locking

To avoid secrets accidentally being written to disk (including swap space), the binary will disable core dumps and lock all allocated memory. The success of this depends on your locally-configured limits (and in particular `RLIMIT_MEMLOCK`) and the capabilities of the binary. To ensure success, by default the binary is given the `CAP_IPC_LOCK` capability during `make install`. This bypasses `RLIMIT_MEMLOCK`.

If for any reason core dumps cannot be disabled or memory cannot be locked, a warning will be generated, unless the binary was compiled with `WARN_ON_MEMORY_LOCK_ERRORS=0`.

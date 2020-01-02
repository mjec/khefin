.TH "m4_APPNAME" 1 "m4_APPDATE" "m4_APPVERSION" "m4_APPNAME man page"

.SH NAME
m4_APPNAME  generate passphrase\-protected secrets from a FIDO2 authenticator that supports the hmac\-secret extension

.SH SYNOPSIS
.B m4_APPNAME
.IR subcommand
[\fIoptions\fR]

.SH SUBCOMMANDS

.B help
print help screen and exit.

.B version
print version and exit.

.B enumerate
show a list of authenticator devices currently connected.

.B enrol
create or overwrite \fIfile\fR with randomly\-generated data required to produce a secret for the given \fIpassphrase\fR, using \fIdeivce\fR.

.B generate
generate an HMAC across the data contained in \fIfile\fR, once it has been decrypted with the given \fIpassphrase\fR.

.SH OPTIONS

.TP
.BR \-d ", " \-\-device =\fIdevice\fR
REQUIRED for the \fBenrol\fR subcommand, otherwise prohibited.
The path to the authenticator to enrol, e.g. /dev/hidraw0; see \fBenumerate\fR.
This device MUST support the FIDO2 hmac\-secret extension (supported by most Yubico Security Keys and YubiKeys).

.TP
.BR \-f ", " \-\-file =\fIfile\fR
REQUIRED for the \fBenrol\fR and \fBgenerate\fR subcommands, otherwise prohibited.
The path to write to (for \fBenrol\fR; this file will be overwritten) or read from (for \fBgenerate\fR).

.TP
.BR \-p ", " \-\-passphrase =\fIpassphrase\fR
Optional for the \fBenrol\fR and \fBgenerate\fR subcommands, otherwise prohibited.
The passphrase to use to encrypt (for \fBenrol\fR) or decrypt (for \fBgenerate\fR) \fIfile\fR.
If not specified, you will be prompted to enter a passphrase.
Note that either way, passphrases must \fBnot\fR contain a null (0x00) byte.

.TP
.BR \-o ", " \-\-obfuscate\-device\-info
Optional for the \fBenrol\fR subcommand, otherwise prohibited.
If specified, do not store the \fIdevice\fR AAGUID (identifier of device make and model) in \fIfile\fR.

.SH DESCRIPTION

m4_APPNAME produces deterministic output which can only be reproduced without \fIfile\fR, the \fIpassphrase\fR and the same authenticator \fIdevice\fR that was used during the \fBenrol\fR step.

The canonical use case for m4_APPNAME is for generating an encryption key (or key material) that depends on two factors (a passphrase you know, and a device you have).

The \fBenumerate\fR subcommand produces a list of connected authenticators, one per line, with the following tab\-separated fields:

.RS
1. a one\-character field, containing a \fB!\fR if the authenticator does not support the hmac\-secret extension and otherwise a blank space

2. the path to the authenticator, e.g. /dev/hidraw0

3. the authenticator name and manufacturer, e.g. Yubico Security Key by Yubico

4. the authenticator AAGUID, e.g. f8a011f3-8c0a-4d15-8006-17111f9edc7d
.RE

.SH EXIT STATUS

.TP
.BR 0
Success

.TP
.BR 32
Bad invocation (invalid \fIsubcommand\fR or \fIoptions\fR)

.TP
.BR 33
Bad passphrase

.TP
.BR 34
No authenticator device connected

.TP
.BR 35
Of the authenticator(s) connected, none could be used (e.g. the connected device is different from the device used for the \fBenrol\fR step)

.TP
.BR 36
\fIfile\fR is corrupt or cannot be decoded

.TP
.BR 64
Out of memory

.TP
.BR 65
Authenticator error

.TP
.BR 66
Cryptography error

.TP
.BR 67
Unable to drop privileges

.TP
.BR 96
This is evidence of a bug; please report it (see \fBBUGS\fR below).

.SH ENVIRONMENT

.TP
.BR FIDO2_HMAC_SECRET_SILENCE_MEMLOCK_ERRORS
If this environment variable is set, errors locking memory will be ignored.
These errors normally only occur if m4_APPNAME is not running as root, or with setuid root; see \fBNOTES\fR below.

.SH FILES

\fIfile\fR (or the key file) contains information essential to producing the output of m4_APPNAME.
Parts of the file are unencrypted, including the authenticator \fIdevice\fR AAGUID (unless \-\-obfuscate\-device\-info is set during the \fBenrol\fR step, this identifies the make and model of device), nonces, and the parameters for the key derivation algorithm used for the encrypted part of the file.

Each key file can be used to produce exactly one secret, given exactly one passphrase and exactly one device.
There is no support for having a backup authenticator for a given file, for example; instead you should create two key files.

If you are using the output of m4_APPNAME for anything, you should keep a backup of \fIfile\fR.
The sensitive components of this file are encrypted with a key derived solely from your \fIpassphrase\fR.
As such, you should \fBnever\fR store your passphrase with the key file.
You can store a backup of the key file in public unencrypted storage without compromising the security of the system per se, though it is good practice to ensure there are reasonable controls preventing public access.

.SH NOTES

By default, m4_APPNAME will be installed as setuid and owned by root.
This is done to ensure it has the CAP_IPC_LOCK capability and no hard RLIMIT_MEMLOCK limit.
m4_APPNAME will disable core dumps and increase RLIMIT_MEMLOCK to 512MiB before dropping privileges to those of the real user ID.
If privileges cannot be dropped, m4_APPNAME will terminate with an appropriate code; see \fBEXIT STATUS\fR above.
The aim of this is to ensure memory is never swapped or dumped to disk, potentially revealing secrets.

.SH SECURITY

This system depends on the security of (and thus can never be more secure than) your authenticator device, the FIDO2 standard, libsodium, HMAC\-SHA256 and your passphrase.

Because the output of m4_APPNAME is the same every time, you should rotate key files regularly.

This also means that m4_APPNAME is not an ideal solution for authentication.
Where possible, you should use a system which relies on a challenge being signed by your authenticator device.
Examples of such systems which use FIDO2 authenticators are WebAuthn and pam_u2f.
In particular, for providing a second factor for sign\-in or privilege escallation using PAM, pam_u2f is a better solution.

.SH BUGS
.UR https://github.com/mjec/fido2\-hmac\-secret/issues
.UE
has an up\-to\-date list of known issues. Bugs can also be reported there.

.SH SEE ALSO

.BR pam_u2f (8)

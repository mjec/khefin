m4_define(`m4_MEMLOCK_WARNINGS_DIVERT_DESTINATION', m4_ifelse(m4_WARN_ON_MEMORY_LOCK_ERRORS, 0, -1, 0))m4_dnl
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
create or overwrite \fIfile\fR with randomly\-generated data required to produce a secret for the given \fIpassphrase\fR, using \fIdevice\fR.

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

.TP
.BR \-k ", " \-\-kdf\-hardness =\fIhardness\fR
Optional for the \fBenrol\fR subcommand, otherwise prohibited.
Specify the complexity of the key derivation function used to derive a cryptographic key from \fIpassphrase\fR.
Valid values for \fIhardness\fR are \fBhigh\fR, \fBmedium\fR or \fBlow\fR.
If not specified, a value will be chosen automatically based on total system RAM.
While greater hardness provides better security (at the cost of CPU time and RAM), more important is that \fIpassphrase\fR is long and difficult to guess.

.TP
.BR \-m ", " \-\-mixin =\fIdata\fR
Optional for the \fBgenerate\fR subcommand, otherwise prohibited.
Combine \fIdata\fR with the encrypted salt, so that the returned value depends on it.
Note that setting \fIdata\fR to an empty string behaves differently to not using this argument at all.

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

If \fIpassphrase\fR is not provided as a command line argument, then behavior depends on whether m4_APPNAME is running at a TTY (interactively) or not.
If m4_APPNAME has a TTY, you will be prompted to enter a passphrase.
If m4_APPNAME does not have a TTY, the passphrase will be read on STDIN, without any prompt, until a newline (or EOF) is reached.
Note that in this case, the newline will \fBnot\fR be included in the passphrase or PIN.

In every case, only the first m4_LONGEST_VALID_PASSPHRASE bytes of a passphrase will be used.

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
.BR 68
Unable to get passphrase safely (no TTY or STDIN)

.TP
.BR 96
This is evidence of a bug; please report it (see \fBBUGS\fR below).

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
If you run m4_APPNAME under \fBsudo\fR(8), it will drop privileges to the invoking user (specified by the \fBSUDO_UID\fR environment variable) after locking memory.

m4_divert(m4_MEMLOCK_WARNINGS_DIVERT_DESTINATION)m4_dnl
If you are seeing errors like \fBUnable to lock memory, which means secrets may be swapped to disk\fR, this means that RLIMIT_MEMLOCK is too low for m4_APPNAME to lock all its memory.
The risk from this is that memory could be swapped to disk, resulting in secrets being written to swap space.
You can fix this by raising RLIMIT_MEMLOCK, running m4_APPNAME as root, or by giving the binary the CAP_IPC_LOCK capability (so it can bypass RLIMIT_MEMLOCK) by running the following command as root:

.RS
setcap cap_ipc_lock+ep /path/to/m4_APPNAME
.RE

m4_divert(0)m4_dnl
The name khefin is a reference to a minor god in the Discworld series of novels by Terry Pratchett.
In that series, Khefin is the two-faced god of gateways, similar to the Roman god Janus.

.SH SECURITY

This system depends on the security of (and thus can never be more secure than) your authenticator device, the FIDO2 standard, libsodium, HMAC\-SHA256 and your passphrase.

Because the output of m4_APPNAME is the same every time, you should rotate key files regularly.

This also means that m4_APPNAME is not an ideal solution for authentication.
Where possible, you should use a system which relies on a challenge being signed by your authenticator device.
Examples of such systems which use FIDO2 authenticators are WebAuthn and pam_u2f.
In particular, for providing a second factor for sign\-in or privilege escallation using PAM, pam_u2f is a better solution.

.SH BUGS
.UR https://github.com/mjec/khefin/issues
The GitHub issues page
.UE
has an up\-to\-date list of known issues. Bugs can also be reported there.

.SH SEE ALSO

.BR pam_u2f (8)
m4_divert(m4_MEMLOCK_WARNINGS_DIVERT_DESTINATION)m4_dnl
.BR setcap (8)
m4_divert(0)m4_dnl
.BR sudo (8)

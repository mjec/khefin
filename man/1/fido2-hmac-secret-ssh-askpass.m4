.TH "m4_APPNAME-ssh-askpass" 1 "m4_APPDATE" "m4_APPVERSION" "m4_APPNAME-ssh-askpass man page"

.SH NAME
m4_APPNAME-ssh-askpass  an ssh-askpass implementation using m4_APPNAME to secure SSH keys

.SH SYNOPSIS
.B m4_APPNAME-ssh-askpass
.B get-passphrase
.IR ssh-keyfile

.SH DESCRIPTION
This is a Bash script that can be used as a drop-in replacement for \fBssh-askpass\fR.
It works by having a passphrase for each SSH key derived using \fB`'m4_APPNAME generate\fR and based on the SSH key's fingerprint.

For this to work, you will need to set \fIssh-keyfile\fR's password to the output of \fB`'m4_APPNAME-ssh-askpass get-passphrase\fI ssh-keyfile\fR.
You may use \fBssh-keygen -p -f \fIssh-keyfile\fB -N "$(m4_APPNAME-ssh-askpass get-passphrase \fIssh-keyfile\fB)"\fR to do this, however be aware that this will make the passphrase (briefly) availble to other users on the system, who can inspect the command line for \fBssh-keygen\fR.

Once that is done, you will need to set the \fBSSH_ASKPASS\fR environment variable to the path to \fB`'m4_APPNAME-ssh-askpass\fR so that \fBssh-agent\fR(1) knows to call it.

.SH NOTES
You should also make a backup of your encrypted keyfile (see \fBFILES\fR below), because without this file you will be unable to decrypt your SSH keys.

This script accepts parameters which are not documented here, in accordance with the way ssh-agent calls it.
In particular, the following ways of calling this script are supported:

.TP
\(bu \fB`'m4_APPNAME-ssh-askpass\fR "Enter passphrase for $keyfile_path:"

.TP
\(bu \fB`'m4_APPNAME-ssh-askpass\fR "Enter passphrase for $keyfile_path (will confirm each use):"

.TP
\(bu \fB`'m4_APPNAME-ssh-askpass\fR "Allow use of key $key_name?\\nKey fingerprint $key_fingerprint."

.SH ENVIRONMENT

.TP
.BR m4_SSH_ASKPASS_ENCRYPTED_KEYFILE_ENV
Path to the encrypted keyfile to use ot protect SSH keys.
Defaults to \fB`'m4_SSH_ASKPASS_DEFAULT_ENCRYPTED_KEYFILE\fR.

.TP
.BR m4_SSH_ASKPASS_NOTIFY_ENV
The command to use to send notifications.
Defaults to \fB`'m4_SSH_ASKPASS_DEFAULT_NOTIFY_COMMAND\fR or to printing to stderr if \fBget-passphrase\fR is used.

.SH FILES

.TP
.BR m4_SSH_ASKPASS_DEFAULT_ENCRYPTED_KEYFILE
The encrypted keyfile to use to protect SSH keys.
Must be created with \fB`'m4_APPNAME`' enrol -p ""\fR.
Can be changed by setting the \fB`'m4_SSH_ASKPASS_NOTIFY_ENV\fR environment variable.
This file should by readable only by its owner, however that is not checked by this application.

.SH BUGS
.UR https://github.com/mjec/fido2\-hmac\-secret/issues
.UE
has an up\-to\-date list of known issues. Bugs can also be reported there.

.SH SEE ALSO

.BR ssh-agent (1)
.BR ssh-keygen (1)
.BR m4_APPNAME (1)

.TH "m4_APPNAME cryptsetup-keyscript" 8 "m4_APPDATE" "m4_APPVERSION" "m4_APPNAME cryptsetup-keyscript man page"

.SH NAME
m4_APPNAME-cryptsetup-keyscript  a Debian-style keyscript for unlocking a LUKS-encrypted disk at boot time with a key derived from a keyfile created with m4_APPNAME

.SH DESCRIPTION
This script, installed to \fI`'m4_INSTALL_PREFIX/lib/m4_APPNAME/cryptsetup-keyscript\fR by default, is designed to be specified as a \fIkeyscript\fR in \fBcrypttab\fR(5).
The \fIkey\fR field in the \fBcrypttab\fR(5) will be used as the keyfile passed to \fB`'m4_APPNAME generate\fR.

In general, to set up disk encryption protected by \fB`'m4_APPNAME\fR(1) you would go through the following steps:

.RS
1. Run \fB`'m4_APPNAME enrol\fR to generate a keyfile.

2. Run \fB`'m4_APPNAME`'-add-luks-key\fR(8) to add a key to your LUKS-encrypted disk.

3. Update \fI/etc/crypttab\fR to specify this keyfile. An example crypttab entry might be:
.RS
sda1_crypt UUID=... /path/to/keyfile luks,keyscript=m4_INSTALL_PREFIX/lib/m4_APPNAME/cryptsetup-keyscript
.RE

4. Run \fBupdate-initramfs -u\fR to update your initramfs.
.RE

.SH NOTES

.B Before using this keyscript, you should be aware of how to boot your computer in the event this does not work correctly.
You should have a backup of your LUKS header, a key slot in your disk that does not rely on m4_APPNAME, and an understanding of how to recover from an unbootable system.

This script requires m4_APPNAME and appropriate libraries to be copied to your initramfs, along with the relevant keyfile.
This is normally handled by the m4_APPNAME initramfs hook script, which is installed to \fI/etc/initramfs-tools/hooks/crypt`'m4_APPNAME\fR by default.
If you are not using \fBinitramfs-tools\fR(8), it is up to you to ensure this happens.

The m4_APPNAME initramfs hook relies on m4_APPNAME being available in the \fBPATH\fR used by \fBmkinitramfs\fR(8), which may not include \fI`'m4_INSTALL_PREFIX/bin\fR.
You may need to edit the hook script to specify the full path to m4_APPNAME, or ensure m4_APPNAME is installed in the appropriate path.

The m4_APPNAME initramfs hook relies on the full path to the keyscript ending with \fB`'m4_APPNAME/cryptsetup-keyscript\fR, wherever it is located.

.SH FILES
.TP
.IR m4_INSTALL_PREFIX/lib/m4_APPNAME/cryptsetup-keyscript
This script.

.TP
.IR /etc/initramfs-tools/hooks/crypt`'m4_APPNAME
The \fBinitramfs-tools\fR(8) hook that ensures this script can run correctly.

.SH BUGS
.UR https://github.com/mjec/khefin/issues
The GitHub issues page
.UE
has an up\-to\-date list of known issues. Bugs can also be reported there.

.SH SEE ALSO

.BR cryptsetup (8)
.BR crypttab (5)
.BR initramfs-tools (8)
.BR m4_APPNAME (1)
.BR m4_APPNAME`'-add-luks-key (8)

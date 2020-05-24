#!/bin/sh

decrypt_`'m4_APPNAME () {
	m4_APPNAME enumerate >/dev/null
	if [ $? -eq 34 ]; then
		>&2 printf "Authenticator device not found!\n"
		return 1
	fi
	if ! /lib/cryptsetup/askpass "Enter passphrase for key $1:" | m4_APPNAME generate -f "$1"; then
		return 1
	fi
	return 0
}

if ! command -v m4_APPNAME > /dev/null; then
         >&2 printf "%s: m4_APPNAME is not available\n" "$0"
        exit 1
fi

if [ -z "$1" ]; then
		>&2 printf "%s: missing first argument (a keyfile)\n" "$0"
        exit 1
fi

decrypt_`'m4_APPNAME "$1"
exit $?

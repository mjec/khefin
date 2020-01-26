#!/bin/bash

set -euo pipefail

prompt="${1-}"
f2hs_keyfile="${m4_SSH_ASKPASS_ENCRYPTED_KEYFILE_ENV-m4_SSH_ASKPASS_DEFAULT_ENCRYPTED_KEYFILE}"

print_usage() {
	progname="$(basename "$0")"
	printf >&2 "%s get-passphrase \"\$keyfile_path\"\n" "$progname"
}

notify() {
	${m4_SSH_ASKPASS_NOTIFY_ENV-m4_SSH_ASKPASS_DEFAULT_NOTIFY_COMMAND} "$1" "$2"
}

get_fingerprint_for_keyfile() {
	if [ ! -f "$1" ]; then
		notify "m4_APPNAME ssh-askpass failure" "No such keyfile: $1"
		exit 1
	fi
	ssh-keygen -l -f "$1" | cut -f 2 -d ' '
}

if [ -z "$prompt" ]; then
	print_usage
	exit 1
fi

if [ ! -r "$f2hs_keyfile" ]; then
	printf >&2 "m4_APPNAME encrypted keyfile does not exist at %s.\nYou can create it with m4_APPNAME enrol." "$f2hs_keyfile"
	exit 1
fi

if [ "$prompt" = "get-passphrase" ]; then
	if [ -z "${m4_SSH_ASKPASS_NOTIFY_ENV-}" ]; then
		notify() {
			>&2 printf "%s\n%s\n" "$1" "$2"
		}
	fi

	if [ -z "${2-}" ]; then
		print_usage
		exit 1
	fi
fi

set +e
m4_APPNAME enumerate >/dev/null 2>&1
if [ $? -eq 34 ]; then
	notify "m4_APPNAME ssh-askpass failure" "No compatible FIDO2 authenticator device is connected."
	exit 1
fi
set -e

if [ "$prompt" = "get-passphrase" ]; then
	fingerprint="$(get_fingerprint_for_keyfile "$2")"
	notify "Trigger authenticator to get passphrase" "Keyfile $2 (fingerprint $fingerprint)."
	m4_APPNAME generate -f "$f2hs_keyfile" -m "$fingerprint" -p ""
elif printf "%s" "$prompt" | grep '^Allow use of key' > /dev/null; then
	notify "Trigger authenticator to allow use of key" "$prompt"
	m4_APPNAME generate -f "$f2hs_keyfile" -m "" -p "" > /dev/null 2>&1
else
	keyfile="$(printf "%s" "$prompt" | sed -E 's/ \(will confirm each use\)(:[[:space:]]*)$/\1/ ; s/Enter passphrase for (.+):.*$/\1/')"
	fingerprint="$(get_fingerprint_for_keyfile "$keyfile")"
	notify "Trigger authenticator to add key to ssh agent" "Keyfile $keyfile (fingerprint $fingerprint)."
	m4_APPNAME generate -f "$f2hs_keyfile" -m "$fingerprint" -p ""
fi

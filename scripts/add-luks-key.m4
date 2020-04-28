#!/bin/bash

set +xv -euo pipefail

# Include a UUID, so this value should never appear otherwise
undefined="undefined 110d43d6-f20a-4f29-bae4-1a58f813980c"

# Include a UUID, so this value should never appear otherwise
decrypted_keyfile_device="8931c13b-8f4f-468e-b372-967e12955c54"

help() {
	printf "Usage: %s <encrypted-keyfile> <disk> [cryptsetup-options]\n\n" "$0"
	fold -w 80 -s <<HELPEOF
This is a Bash script that runs cryptsetup's luksAddKey, passing in a keyfile generated with m4_APPNAME.  It is designed for use with the m4_APPNAME mkinitcpio hook.

encrypted-keyfile must have been generated by m4_APPNAME enrol.

This script must be run as root.

!!! Make an off-system backup of your LUKS header for <disk> before running this command. !!!

You should also make a backup of encrypted-keyfile, because without this file the new keyslot on disk cannot be used.
HELPEOF
}

cleanup() {
	# Try really hard to clean up, even if part of cleanup fails
	set +e
	cryptsetup close "$decrypted_keyfile_device" 2>/dev/null
	[ "${loopback_device-$undefined}" != "$undefined" ] && losetup -d "$loopback_device" 2>/dev/null
	rm -f /tmp/"$decrypted_keyfile_device".* 2>/dev/null
	unset encrypted_keyfile_passphrase
	unset raw_key
}

trap cleanup EXIT TERM INT

if [ "$#" -lt 2 ] || [ "$(id -u)" -ne 0 ]; then
	help
	exit 1
fi

stty -echo
printf "Enter your passphrase for %s: " "$1"
read -r encrypted_keyfile_passphrase
stty echo
printf "\n"
raw_key="$(printf "%s" "$encrypted_keyfile_passphrase" | m4_APPNAME generate -f "$1")"
unset encrypted_keyfile_passphrase
# shellcheck disable=SC2003
raw_key_length="$(expr length "$raw_key" + 1)"
loopback_device="$(losetup -f)"
backing_file=$(mktemp -t "$decrypted_keyfile_device.XXXXXXXX")
dd if=/dev/zero of="$backing_file" bs=1024 count=64 2>/dev/null
losetup "$loopback_device" "$backing_file"
# Get a random passphrase and pipe it into cryptsetup open. We don't need to keep this.
# Use `tr` to remove control characters, DEL (0o177) and 0xff (0o377).
# This means removing 33 characters of 256 possibilities, or ~13% of the input.
# We generate 600 characters of input, thus expect a ~522 byte useful passphrase.
# If the output of /dev/urandom is non-uniform, we have bigger problems ;).
dd if=/dev/urandom bs=600 count=1 2>/dev/null \
	| tr -d "\000-\037\177\377" \
	| cryptsetup open --type plain "$loopback_device" "$decrypted_keyfile_device"
decrypted_keyfile_path="/dev/mapper/$decrypted_keyfile_device"
printf "%s\n" "$raw_key" > "$decrypted_keyfile_path"
unset raw_key

cryptsetup luksAddKey "$2" "${@:3}" --new-keyfile-size "$raw_key_length" "$decrypted_keyfile_path"
unset raw_key_length

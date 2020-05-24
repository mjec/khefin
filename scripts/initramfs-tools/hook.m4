#!/bin/sh

`#' Change this to a full path if m4_APPNAME is not installed on the mkinitramfs path. E.g.:
`#' path_to_`'m4_APPNAME="/usr/local/bin/m4_APPNAME"
path_to_`'m4_APPNAME="$(command -v m4_APPNAME)"

PREREQ="cryptroot"

prereqs() {
	echo "$PREREQ"
}

case $1 in
prereqs)
	prereqs
	exit 0
	;;
esac

#shellcheck disable=SC1091
. /usr/share/initramfs-tools/hook-functions
#shellcheck disable=SC1091
. /lib/cryptsetup/functions

if ! [ -f "$TABFILE" ]; then
	>&2 printf "Skipping hook because no crypttab file at %s\n" "$TABFILE"
	exit 0
fi

copy_app() {
	if ! copy_exec "$path_to_`'m4_APPNAME"; then
		>&2 printf "Unable to find m4_APPNAME on PATH (%s).\n" "$PATH"
		>&2 printf "Ensure m4_APPNAME is installed into that path, or hard-code its path at the top of %s.\n" "$0"
		exit 1
	fi
}

copy_key() {
	crypttab_parse_options

	# using case for wildcard matching
	case "${CRYPTTAB_OPTION_keyscript-}" in
		*m4_APPNAME/cryptsetup-keyscript)
			if [ "${CRYPTTAB_KEY-none}" = "none" ]; then
				>&2 printf "m4_APPNAME keyscript specified but no key specified!\n"
				exit 1
			fi

			if ! [ -f "$CRYPTTAB_KEY" ]; then
				>&2 printf "m4_APPNAME keyscript specified but key at %s does not exist.\n" "$CRYPTTAB_KEY"
				exit 1
			fi

			copy_app
			copy_file "key" "$CRYPTTAB_KEY"
			if [ $? -gt 1 ]; then
				exit 1
			fi
			;;
	esac
}

crypttab_foreach_entry copy_key

exit 0

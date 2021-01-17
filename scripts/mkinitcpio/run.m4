m4_define(`m4_PROMPT_NEVER', 0)m4_dnl
m4_define(`m4_PROMPT_ONCE', 1)m4_dnl
m4_define(`m4_PROMPT_ALWAYS', 2)m4_dnl
#!/usr/bin/ash
# shellcheck shell=dash

# Include a UUID in these, so this value should never appear otherwise
undefined="undefined 110d43d6-f20a-4f29-bae4-1a58f813980c"
ramfs_mount_point="/tmp/m4_APPNAME-8931c13b-8f4f-468e-b372-967e12955c54"
encrypted_keyfile_passphrase_file="$ramfs_mount_point/c43ae4e3-9a88-475f-a178-2adf37fe635a"
disk_encryption_key_file="$ramfs_mount_point/5a7b3b6c-d8d0-435d-929d-82d9f3d6b8e9"

run_hook() {
	umask 077
	encrypted_keyfile_dir=${encrypted_keyfile_dir-m4_INITCPIO_DEFAULT_ENCRYPTED_KEYFILE_DIR}

	`#' From m4_APPNAME man page, EXIT CODES section:
	`#'   34     No authenticator device connected
	m4_APPNAME enumerate > /dev/null
	if [ "$?" -eq 34 ]; then
		if [ "${do_not_prompt_for_authenticator:-$undefined}" != "$undefined" ]; then
			printf "No authenticator device found; skipping m4_APPNAME.\n"
			return
		else
			printf "Insert your authenticator device and press any key to continue.\n"
			printf "If you press a key without inserting your authenticator, the m4_APPNAME hook will be skipped.\n"
			stty -icanon -echo
			dd bs=1 count=1 2>/dev/null
			stty icanon echo
			m4_APPNAME enumerate > /dev/null
			if [ "$?" -eq 34 ]; then
				printf "No authenticator device found; skipping m4_APPNAME hook.\n"
				return
			fi
		fi
	fi

	if [ -d "$ramfs_mount_point" ] && [ -n "$(ls -A "$ramfs_mount_point")" ]; then
		printf "%s unexpectedly exists and is not an empty directory; skipping m4_APPNAME hook.\n" "$ramfs_mount_point" 
	elif [ -e "$ramfs_mount_point" ]; then
		printf "%s unexpectedly exists and is not an empty directory; skipping m4_APPNAME hook.\n" "$ramfs_mount_point" 
	fi

	mkdir -p "$ramfs_mount_point"
	mount -t ramfs -o size=64k "$ramfs_mount_point" "$ramfs_mount_point"

	if [ "${encrypted_keyfile_passphrase-$undefined}" != "$undefined" ]; then
		# Never prompt
		passphrase_prompt_option=m4_PROMPT_NEVER
		printf "Using hardcoded passphrase for m4_APPNAME.\n"
	elif [ "${same_passphrase_every_keyfile:-$undefined}" != "$undefined" ]; then
		# Prompt first time only
		passphrase_prompt_option=m4_PROMPT_ONCE
	else
		# Always prompt
		passphrase_prompt_option=m4_PROMPT_ALWAYS
	fi

	# This ! [ $x -gt 0 ] construction means that we do the body even if $x is not a number
	if ! [ "${encrypted_keyfile_passphrase_attempts-NaN}" -gt 0 ] > /dev/null 2>&1; then
		encrypted_keyfile_passphrase_attempts=m4_INITCPIO_DEFAULT_MAX_PASSPHRASE_ATTEMPTS
	fi

	for encrypted_keyfile in "$encrypted_keyfile_dir"/* "$encrypted_keyfile_dir"/.*; do
		if [ ! -f "$encrypted_keyfile" ]; then
			continue
		fi

		passphrase_tries=0
		exit_loop=0

		if [ "$passphrase_prompt_option" -eq m4_PROMPT_ALWAYS ]; then
			# Prompt for a new passphrase for this keyfile
			unset encrypted_keyfile_passphrase
		fi

		while
			passphrase_tries=$((passphrase_tries + 1))
			if [ "${encrypted_keyfile_passphrase-$undefined}" = "$undefined" ]; then
				stty -echo
				case $passphrase_prompt_option in
					m4_PROMPT_ONCE) printf "Enter your passphrase for all keyfiles in %s: " "$encrypted_keyfile_dir" ;;
					m4_PROMPT_ALWAYS) printf "Enter your passphrase for %s: " "$encrypted_keyfile" ;;
				esac
				read -r encrypted_keyfile_passphrase
				stty echo
				printf "\n"
			fi

			if [ $passphrase_prompt_option -ne m4_PROMPT_ALWAYS ]; then
				printf "Trying to decrypt %s.\n" "$encrypted_keyfile"
			fi

			printf "%s" "$encrypted_keyfile_passphrase" > "$encrypted_keyfile_passphrase_file"
			m4_APPNAME generate -f "$encrypted_keyfile" -r "$encrypted_keyfile_passphrase_file" > $disk_encryption_key_file
			result=$?
			: < /dev/null > $encrypted_keyfile_passphrase_file

			if [ $result -eq 0 ]; then
				export cryptkey="rootfs:$disk_encryption_key_file"
				return
			elif [ $result -eq 33 ] && [ $passphrase_prompt_option -ne m4_PROMPT_NEVER ]; then
				`#' From m4_APPNAME man page, EXIT CODES section:
				`#'   33     Bad passphrase
				unset encrypted_keyfile_passphrase
			else
				# Failure but not due to a bad passphrase, so move on to the next keyfile
				exit_loop=1
			fi

			if [ $passphrase_tries -ge "$encrypted_keyfile_passphrase_attempts" ]; then
				printf "Too many failed passphrase attempts (maximum of %s).\n" $encrypted_keyfile_passphrase_attempts
				exit_loop=1
			fi

			[ $exit_loop -eq 0 ]
		do :; done
	done

	unset encrypted_keyfile_passphrase
}

run_cleanuphook() {
	: < /dev/null > $disk_encryption_key_file
	rm -f "$disk_encryption_key_file"
	rm -f "$encrypted_keyfile_passphrase_file"
	umount -f "$ramfs_mount_point"
	rm -rf "$ramfs_mount_point"
}

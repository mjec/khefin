m4_define(`m4_PROMPT_NEVER', 0)m4_dnl
m4_define(`m4_PROMPT_ONCE', 1)m4_dnl
m4_define(`m4_PROMPT_ALWAYS', 2)m4_dnl
#!/bin/bash

undefined="undefined 110d43d6-f20a-4f29-bae4-1a58f813980c"

get_file_paths_from_env() {
	export encrypted_keyfile_dir=${encrypted_keyfile_dir-m4_DEFAULT_ENCRYPTED_KEYFILE_DIR}
	if [ "${cryptkey-$undefined}" = "$undefined" ]; then
		return
	fi
	IFS=: read -r ckdev ckarg1 <<EOF
$cryptkey
EOF
	if [ "$ckdev" = "rootfs" ]; then
		export decrypted_keyfile_path=$ckarg1
	fi
}

run_hook() {
	get_file_paths_from_env

	if [ "${decrypted_keyfile_path-$undefined}" = "$undefined" ]; then
		return
	fi

	# From m4_APPNAME man page, EXIT CODES section:
	#   34     No authenticator device connected
	m4_APPNAME enumerate > /dev/null
	if [ "$?" -eq 34 ]; then
		return
	fi

	if [ "${encrypted_keyfile_passphrase-$undefined}" != "$undefined" ]; then
		# Never prompt
		passphrase_prompt_option=m4_PROMPT_NEVER
		printf "Using handcoded passphrase for m4_APPNAME.\n"
	elif [ "${same_passphrase_every_keyfile:-$undefined}" != "$undefined" ]; then
		# Prompt first time only
		passphrase_prompt_option=m4_PROMPT_ONCE
	else
		# Always prompt
		passphrase_prompt_option=m4_PROMPT_ALWAYS
	fi

	# This ! [ $x -gt 0 ] construction means that we do the body even if $x is not a number
	if ! [ "${encrypted_keyfile_passphrase_attempts-NaN}" -gt 0 ] > /dev/null 2>&1; then
		encrypted_keyfile_passphrase_attempts=m4_DEFAULT_MAX_PASSPHRASE_ATTEMPTS
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
					1) printf "Enter your passphrase for all keyfiles in %s: " "$encrypted_keyfile_dir" ;;
					2) printf "Enter your passphrase for %s: " "$encrypted_keyfile" ;;
				esac
				read -r encrypted_keyfile_passphrase
				stty echo
				printf "\n"
			fi

			if [ $passphrase_prompt_option -ne m4_PROMPT_ALWAYS ]; then
				printf "Trying to decrypt %s.\n" "$encrypted_keyfile"
			fi

			printf "%s" "$encrypted_keyfile_passphrase" | m4_APPNAME generate -f "$encrypted_keyfile" > "$decrypted_keyfile_path"
			result=$?
			if [ $result -eq 0 ]; then
				unset encrypted_keyfile_passphrase
				return
			elif [ $result -eq 33 ] && [ $passphrase_prompt_option -ne m4_PROMPT_NEVER ]; then
				# From m4_APPNAME man page, EXIT CODES section:
				#   33     Bad passphrase
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
	get_file_paths_from_env

	if [ ! -f "$decrypted_keyfile_path" ]; then
		return
	fi

	dd if=/dev/zero of="$decrypted_keyfile_path" bs=1 count="$(stat -c %s "$decrypted_keyfile_path")" 2>/dev/null
	rm -f "$decrypted_keyfile_path"
}

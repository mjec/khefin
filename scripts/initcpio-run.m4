m4_define(`m4_PROMPT_NEVER', 0)m4_dnl
m4_define(`m4_PROMPT_ONCE', 1)m4_dnl
m4_define(`m4_PROMPT_ALWAYS', 2)m4_dnl
#!/bin/sh

# Include a UUID, so this value should never appear otherwise
undefined="undefined 110d43d6-f20a-4f29-bae4-1a58f813980c"

# Include a UUID, so this value should never appear otherwise
decrypted_keyfile_device="8931c13b-8f4f-468e-b372-967e12955c54"

run_hook() {
	encrypted_keyfile_dir=${encrypted_keyfile_dir-m4_DEFAULT_ENCRYPTED_KEYFILE_DIR}

	# if [ "${decrypted_keyfile_path-$undefined}" = "$undefined" ]; then
	# 	return
	# fi

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

			raw_key="$(printf '%s' "$encrypted_keyfile_passphrase" | m4_APPNAME generate -f "$encrypted_keyfile")"
			result=$?
			raw_key_length="$(expr length "$raw_key" + 1)"

			if [ $result -eq 0 ]; then
				unset encrypted_keyfile_passphrase
				m4_translit(m4_APPNAME, `-', `_')_loopback_device="$(losetup -f)"
				export m4_translit(m4_APPNAME, `-', `_')_loopback_device
				backing_file=$(mktemp -t "$decrypted_keyfile_device.XXXXXXXX")
				dd if=/dev/zero of="$backing_file" bs=1024 count=64 2>/dev/null
				losetup "$m4_translit(m4_APPNAME, `-', `_')_loopback_device" "$backing_file"
				# Get a random passphrase and pipe it into cryptsetup open. We don't need to keep this.
				# Use `tr` to remove control characters, DEL (0o177) and 0xff (0o377).
				# This means removing 33 characters of 256 possibilities, or ~13% of the input.
				# We generate 600 characters of input, thus expect a ~522 byte useful passphrase.
				# If the output of /dev/urandom is non-uniform, we have bigger problems ;).
				dd if=/dev/urandom bs=600 count=1 2>/dev/null \
					| tr -d "\000-\037\177\377" \
					| cryptsetup open --type plain "$m4_translit(m4_APPNAME, `-', `_')_loopback_device" "$decrypted_keyfile_device"
				decrypted_keyfile_path="/dev/mapper/$decrypted_keyfile_device"
				printf "%s\n" "$raw_key" > "$decrypted_keyfile_path"
				unset raw_key
				export cryptkey="$decrypted_keyfile_path:0:$raw_key_length"
				unset raw_key_length
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
	cryptsetup close "$decrypted_keyfile_device"
	losetup -d "$m4_translit(m4_APPNAME, `-', `_')_loopback_device"
	rm -f /tmp/"$decrypted_keyfile_device".*
}

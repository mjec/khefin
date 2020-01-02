#!/bin/bash

build() {
	keyfiles_source_dir="${keyfiles_source_dir-m4_DEFAULT_KEYFILES_SOURCE_DIR}"
	encrypted_keyfile_dir="${encrypted_keyfile_dir-m4_DEFAULT_ENCRYPTED_KEYFILE_DIR}"

	# If $keyfiles_source_dir doesn't exist, we can't copy over any keyfiles, and this hook should never run
	if [ ! -d "$keyfiles_source_dir" ]; then
		return
	fi

	# If $keyfiles_source_dir is empty, we can't copy over keyfiles, and this hook should never run
	if [ "$(find "$keyfiles_source_dir" -maxdepth 1 -type f -print | wc -l)" -eq 0 ]; then
		return
	fi

	add_binary m4_APPNAME
	add_binary stty

	find "$keyfiles_source_dir" -maxdepth 1 ! -name "$(printf "*\n*")" -type f -print \
	| while IFS= read -r keyfile; do
		add_file "$keyfile" "$encrypted_keyfile_dir"/"$(basename "$keyfile")" 400 
	done

	add_runscript
}

help() {
	fold -w 80 -s <<HELPEOF
This hook allows for generating a keyfile for an encrypted root device using m4_APPNAME. It requires that you have zero or more keyfiles (each created by running m4_APPNAME enrol) stored in \$keyfiles_source_dir (using the value at the time you run mkinitcpio, or the default m4_DEFAULT_KEYFILES_SOURCE_DIR). Those are tried in alphabetical order, and every regular file in that directory (though not in any subdirectories) is tried.

You can use a different passphrase for every keyfile, or set the kernel parameter same_passphrase_every_keyfile to use a single passphrase for every file. You can hardcode a passphrase with the kernel parameter encrypted_keyfile_passphrase, and change the number of passphrase attempts permitted by setting the encrypted_keyfile_passphrase_attempts variable (this defaults to m4_DEFAULT_MAX_PASSPHRASE_ATTEMPTS).

Those files are copied to \$encrypted_keyfile_dir in the initcpio image, which you can set by a kernel parameter. By default that directory is m4_DEFAULT_ENCRYPTED_KEYFILE_DIR.

To make use of this hook, it must be set to run BEFORE the encrypt hook, and you must be using a keyfile, specified by cryptkey=rootfs:/tmp/keyfile (or some other path to a cryptsetup keyfile). NOTE: that cryptsetup keyfile path WILL BE OVERWRITTEN with the output of m4_APPNAME generate, or the file deleted if generate never succeeds. The only exception is if there are no attached devices, in which case the file will never be created/will be left alone.

You will also need to add these keys to a key slot for your disk by running the following commands (with relevant path substitutions as required, and after backing up your LUKS header):
	m4_APPNAME generate \\
		-f "\$keyfiles_source_dir/your-keyfile" \\
		> /tmp/keyfile
	sudo cryptsetup -v luksAddKey \\
		/dev/your-disk \\
		/tmp/keyfile
	shred /tmp/keyfile && rm /tmp/keyfile

If an authenticator is not plugged in, or you get the passphrase wrong three times, this hook will not write your cryptsetup keyfile and the encrypt hook will fall back to prompting you for a passphrase. Note that you MUST plug the authenticator in BEFORE the udev hook runs, i.e. before switching on your computer.
HELPEOF
}

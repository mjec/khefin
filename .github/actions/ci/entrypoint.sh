#!/bin/bash

case "$1" in
	"format")
		make check-format
		;;
	"lint")
		make check-lint
		;;
	"shellcheck")
		make shellcheck
		;;
	"clang")
		make all -j
		;;
	"gcc")
		CC=gcc make all -j
		;;
	*)
		echo "Unrecognized command"
		exit 1
		;;
esac

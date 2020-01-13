#!/bin/bash

case "$1" in
	"format")
		make format && git diff --exit-code
		;;
	"lint")
		make lint
		;;
	"shellcheck")
		make shellcheck
		;;
	"clang")
		make all
		;;
	"gcc")
		CC=gcc make all
		;;
	*)
		echo "Unrecognized command"
		exit 1
		;;
esac
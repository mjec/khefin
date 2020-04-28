#include "files.h"
#include "exit.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

encoded_file *read_file(const char *path) {
	encoded_file *result =
	    malloc_or_exit(sizeof(encoded_file), "encoded file structure");

	long int ftell_result;

	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to open file at %s", path);
	}

	if (fseek(fp, 0, SEEK_END) != 0) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to seek to end of file at %s",
		     path);
	}

	if ((ftell_result = ftell(fp)) < 0) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to get size of file at %s",
		     path);
	}
	size_t length = (size_t)ftell_result;

	if (fseek(fp, 0, SEEK_SET) != 0) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to seek to start of file at %s", path);
	}

	if (length > LARGEST_VALID_PAYLOAD_SIZE_BYTES) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "File at %s is too big (more than %d bytes); refusing to load it",
		     path, LARGEST_VALID_PAYLOAD_SIZE_BYTES);
	}

	unsigned char *buffer =
	    malloc_or_exit(length, "buffer for reading keyfile");

	if (fread(buffer, length, 1, fp) != 1) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to read file at %s into buffer", path);
	}

	fclose(fp);

	result->path = malloc_or_exit(strlen(path), "encoded file path");
	strncpy(result->path, path, strlen(path));
	result->data = buffer;
	result->length = length;

	return result;
}

void write_file(encoded_file *file) {
	FILE *fp = fopen(file->path, "w");

	if (fp == NULL) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to open file at %s",
		     file->path);
	}

	if (fseek(fp, 0, SEEK_SET) != 0) {
		errx(EXIT_DESERIALIZATION_ERROR,
		     "Unable to seek to start of file at %s", file->path);
	}

	if (fwrite(file->data, file->length, 1, fp) != 1) {
		errx(EXIT_DESERIALIZATION_ERROR, "Unable to write file at %s",
		     file->path);
	}

	fclose(fp);
}

void free_encoded_file(encoded_file *file) {
	if (file->path) {
		free(file->path);
	}
	if (file->data) {
		free(file->data);
	}
	free(file);
}

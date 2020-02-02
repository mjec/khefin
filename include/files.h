#ifndef FILES_H
#define FILES_H

#include "serialization_types.h"

#ifndef LARGEST_VALID_PAYLOAD_SIZE_BYTES
#define LARGEST_VALID_PAYLOAD_SIZE_BYTES 10485760
#endif

encoded_file *read_file(const char *path);
void write_file(encoded_file *file);
void free_encoded_file(encoded_file *file);

#endif

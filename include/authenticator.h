#ifndef fido2hmacsecret_authenticator_H
#define fido2hmacsecret_authenticator_H

#define RELYING_PARTY_ID_SIZE 32
#define RELYING_PARTY_SUFFIX ".v1.fido2-hmac-secret.localhost"
#define RELYING_PARTY_SUFFIX_SIZE strlen(RELYING_PARTY_SUFFIX)

#ifndef MAX_DEVICES_TO_LIST
#define MAX_DEVICES_TO_LIST 64
#endif

#define CLIENT_DATA_HASH_SIZE_BYTES 32

#include <fido.h>

typedef struct secret_t {
	unsigned char *secret;
	size_t secret_size;
} secret_t;

typedef struct authenticator_parameters_t {
	unsigned char *user_id;
	size_t user_id_size;
	unsigned char *client_data_hash;
	size_t client_data_hash_size;
	char *user_name;
	char *user_display_name;
	char *relying_party_id;
	char *relying_party_name;
	unsigned char *credential_id;
	size_t credential_id_size;
	unsigned char *salt;
	size_t salt_size;
} authenticator_parameters_t;

typedef struct devices_list_t {
	size_t count;
	fido_dev_info_t *list;
} devices_list_t;

devices_list_t *list_devices(void);
void free_devices_list(devices_list_t *devices_list);

fido_dev_t *get_device(const char *path);
bool device_supports_hmac_secret(fido_cbor_info_t *device_info);
fido_cbor_info_t *get_device_info(fido_dev_t *device);
void free_device_info(fido_cbor_info_t *cbor_info);
void close_and_free_device_ignoring_errors(fido_dev_t *device);
void close_and_free_device(fido_dev_t *device);

authenticator_parameters_t *
allocate_parameters_except_rpid(size_t credential_id_size, size_t salt_size);
/**
 * This function assumes all pointers are to malloc()'d memory
 * except relying_party_id, credential_id and salt, which must
 * be malloc()'d.
 */
void free_parameters(authenticator_parameters_t *params);

void create_credential(fido_dev_t *device, authenticator_parameters_t *params);

secret_t *allocate_secret(size_t size);
int get_secret_from_authenticator_params(fido_dev_t *device,
                                         authenticator_parameters_t *params,
                                         secret_t *secret_struct);
void free_secret(secret_t *secret_struct);

#endif

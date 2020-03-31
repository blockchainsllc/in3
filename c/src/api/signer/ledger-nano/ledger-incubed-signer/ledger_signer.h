// @PUBLIC_HEADER


#ifndef in3_ledger_signer_h__
#define in3_ledger_signer_h__

#include "../../../../../include/in3/client.h"
#define LEDGER_NANOS_VID 0x2C97
#define LEDGER_NANOS_PID 0x1001

in3_ret_t is_ledger_device_connected();
in3_ret_t eth_ledger_get_public_key(bytes_t i_bip_path,  bytes_t o_public_key);
in3_ret_t eth_get_address_from_path(bytes_t i_bip_path,  bytes_t o_address);
in3_ret_t eth_ledger_sign(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);
in3_ret_t eth_ledger_set_signer(in3_t* in3);

#endif
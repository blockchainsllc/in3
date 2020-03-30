// @PUBLIC_HEADER


#ifndef in3_ledger_signer_h__
#define in3_ledger_signer_h__

#include "client.h"
#define LEDGER_NANOS_VID 0x2C97
#define LEDGER_NANOS_PID 0x1001

in3_ret_t is_ledger_device_connected();
in3_ret_t eth_get_address_from_path(uint8_t[] bip_path );
in3_ret_t eth_sign_msg();
in3_ret_t eth_sign_txn();
in3_ret_t eth_set_ledger_signer(in3_t* in3);

#endif
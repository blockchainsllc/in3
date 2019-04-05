/** @file 
 * USN API.
 * 
 * This header-file defines easy to use function, which are verifying USN-Messages.
 * */

#ifndef USN_API_H
#define USN_API_H

#include "../core/client/client.h"
#include "../core/util/data.h"
#include "../core/util/utils.h"

typedef enum {
  USN_ACTION,
  USN_REQUEST,
  USN_RESPONSE

} usn_msg_type_t;

typedef struct {
  bool           accepted;
  char*          error_msg;
  char*          action;
  usn_msg_type_t msg_type;
  unsigned int   id;
} usn_msg_result_t;

typedef struct {
  char*     device_url;
  address_t contract;
  bytes32_t device_id;
  uint64_t  chain_id;
  uint64_t  now;
} usn_device_conf_t;

usn_msg_result_t usn_verify_message(in3_t* c, char* message, usn_device_conf_t* conf);

#endif
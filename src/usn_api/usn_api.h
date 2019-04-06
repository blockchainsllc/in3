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
  bytes32_t tx_hash;
  uint64_t  rented_from;
  uint64_t  rented_until;
  uint8_t*  controller;
  uint64_t  props[16];
} usn_booking_t;

typedef struct {
  char*          url;
  bytes32_t      id;
  int            num_bookings;
  usn_booking_t* bookings;
} usn_device_t;

typedef struct {
  usn_device_t* devices;
  int           len_devices;
  address_t     contract;
  uint64_t      chain_id;
  uint64_t      now;
  uint64_t      last_checked_block;
  in3_t*        c;
} usn_device_conf_t;

typedef struct {
  bool           accepted;
  char*          error_msg;
  char*          action;
  usn_msg_type_t msg_type;
  unsigned int   id;
  usn_device_t*  device;
} usn_msg_result_t;

typedef struct {
  bytes32_t device_id;
  char*     contract_name;
  uint64_t  counter;
} usn_url_t;

typedef enum {
  BOOKING_NONE,
  BOOKING_START,
  BOOKING_STOP
} usn_event_type_t;

typedef struct {
  uint64_t         ts;
  usn_device_t*    device;
  usn_event_type_t type;
} usn_event_t;

usn_msg_result_t usn_verify_message(usn_device_conf_t* conf, char* message);
int              usn_register_device(usn_device_conf_t* conf, char* url);
usn_url_t        usn_parse_url(char* url);
int              usn_update_bookings(usn_device_conf_t* conf);
void             usn_remove_old_bookings(usn_device_conf_t* conf);
usn_event_t      usn_get_next_event(usn_device_conf_t* conf);
#endif
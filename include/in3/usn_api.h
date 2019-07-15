// @PUBLIC_HEADER
/** @file
 * USN API.
 * 
 * This header-file defines easy to use function, which are verifying USN-Messages.
 * */

#ifndef USN_API_H
#define USN_API_H

#include "client.h"

typedef enum {
  USN_ACTION,
  USN_REQUEST,
  USN_RESPONSE

} usn_msg_type_t;

typedef struct {
  bytes32_t tx_hash;
  uint64_t  rented_from;
  uint64_t  rented_until;
  address_t controller;
  uint64_t  props[16];
} usn_booking_t;

typedef struct {
  char*          url;
  bytes32_t      id;
  int            num_bookings;
  usn_booking_t* bookings;
  int            current_booking;
} usn_device_t;

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

typedef int (*usn_booking_handler)(usn_event_t*);

typedef struct {
  in3_t*              c;
  address_t           contract;
  usn_device_t*       devices;
  int                 len_devices;
  uint64_t            chain_id;
  uint64_t            now;
  uint64_t            last_checked_block;
  usn_booking_handler booking_handler;
} usn_device_conf_t;

usn_msg_result_t usn_verify_message(usn_device_conf_t* conf, char* message);
in3_ret_t        usn_register_device(usn_device_conf_t* conf, char* url);
usn_url_t        usn_parse_url(char* url);

unsigned int usn_update_state(usn_device_conf_t* conf, unsigned int wait_time);
in3_ret_t    usn_update_bookings(usn_device_conf_t* conf);
void         usn_remove_old_bookings(usn_device_conf_t* conf);
usn_event_t  usn_get_next_event(usn_device_conf_t* conf);

in3_ret_t usn_rent(in3_t* c, address_t contract, address_t token, char* url, uint32_t seconds, bytes32_t tx_hash);
in3_ret_t usn_return(in3_t* c, address_t contract, char* url, bytes32_t tx_hash);
in3_ret_t usn_price(in3_t* c, address_t contract, address_t token, char* url, uint32_t seconds, address_t controller, bytes32_t price);

#endif

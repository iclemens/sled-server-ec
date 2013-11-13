#ifndef __ETHERCAT_INTERNAL_H__
#define __ETHERCAT_INTERNAL_H__

#include "ethercat.h"
#include <stdint.h>

static const uint16_t ETHERCAT_TYPE = 0x88A4;


enum payload_type_t
{
  pt_datagram = 0x01
};


enum command_type_t
{
  cmd_noop = 0x00,
  cmd_ainc_r,
  cmd_ainc_w,
  cmd_ainc_rw,

  cmd_cadr_r,
  cmd_cadr_w,
  cmd_cadr_rw,

  cmd_bcst_r,
  cmd_bcst_w,
  cmd_bcst_rw,

  cmd_lgcl_r,
  cmd_lgcl_w,
  cmd_lgcl_rw,

  cmd_cadr_rmwr
};


struct ethercat_header_t
{
  uint8_t dst_addr[6];
  uint8_t src_addr[6];
  uint16_t proto_type;

  uint16_t length;
  payload_type_t type;
};

struct datagram_header_t
{
	uint8_t command;
	uint8_t index;
	address_t address;

	uint16_t length: 11;
	unsigned int flags: 5;
	
	uint16_t interrupt;
} __attribute__ ((packed));

struct datagram_t
{
	datagram_header_t *header;
	uint8_t *payload;
	uint16_t *wkc;
};

static const char *command_description[] = {
  "No operation",
  "Auto increment read",
  "Auto increment write",
  "Auto increment read/write",
  "Configured address read",
  "Configured address write",
  "Configured address read/write",
  "Broadcast read",
  "Broadcast write",
  "Broadcast read/write",
  "Logical memory read",
  "Logical memory write",
  "Logical memory read/write",
  "Configured address read/multiple write",
  "Unrecognized (0xE)",
  "Unrecognized (0xF)"
};


struct ethercat_operation_t
{
	command_type_t command;
	address_t address;
	uint16_t length;
	int flags;

	ec_read_callback_t *read_callback;
	ec_write_callback_t *write_callback;
	void *payload;

	ethercat_operation_t *prev;
	ethercat_operation_t *next;
};

struct ethercat_t
{
	int socket;

	ethercat_operation_t *operations;
};


#endif


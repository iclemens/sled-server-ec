#ifndef __EC_TYPES_H__
#define __EC_TYPES_H__


static const uint16_t ETHERCAT_TYPE = 0x88A4;


/**
 * Standard ethernet header
 */
struct ethernet_header_t
{
	uint8_t dst_addr[6];
	uint8_t src_addr[6];
	uint16_t type;
} __attribute__ ((packed));


struct ethernet_footer_t
{
	uint32_t fcs;
} __attribute__ ((packed));


/**
 * Payload header
 */
struct payload_header_t
{
	/**
	 * 11-bits length
	 * 1-bit type
	 * 4-bits type (01 for datagram)
	 */
	uint16_t length_and_type;
} __attribute__ ((packed));


/**
 *
 */
struct datagram_header_t
{
	uint8_t command;
	uint8_t index;
	uint32_t address;

	/**
	 * 11-bits lenth
	 * 2-bits unused
	 * 1-bit circulating frame
	 * 1-bit unused
	 * 1-bit more frames following
	 */
	uint16_t length_and_flags;
} __attribute__ ((packed));


struct datagram_footer_t
{
	/* Counts the number of read/write interactions with the datagram */
	uint16_t working_counter;
} __attribute__ ((packed));


enum ec_command
{
	CMD_NOOP = 0x00,
	CMD_AI_READ,
	CMD_AI_WRITE,
	CMD_AI_READWRITE,
	CMD_CA_READ,
	CMD_CA_WRITE,
	CMD_CA_READWRITE,
	CMD_BR_READ,
	CMD_BR_WRITE,
	CMD_BR_READWRITE,
	CMD_LO_READ,
	CMD_LO_WRITE,
	CMD_LO_READWRITE,
	CMD_CA_READMWRITE,
	CMD_RESERVED
};

#endif


#include "ethercat.h"
#include "ethercat_internal.h"
#include "ethercat_socket.h"

#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void decode(uint8_t *buffer);


/*****************************
 * Constructor and destructor
 */

ethercat_t *ec_create(const char *device)
{	
	struct ethercat_t *ethercat = 
		(struct ethercat_t *) malloc(sizeof(struct ethercat_t));

	if(ethercat == NULL) {
		perror("malloc()");
		return NULL;
	}

	ethercat->socket = open_socket(device);
	ethercat->operations = NULL;

	if(ethercat->socket == -1) {
		free(ethercat);
		return NULL;
	}

	return ethercat;
}


void ec_destroy(ethercat_t **ethercatv)
{
	struct ethercat_t *ethercat = *ethercatv;

	if(ethercat) {
		close(ethercat->socket);
		free(ethercat);
	}
	*ethercatv = NULL;
}


/*********************
 * Adding read/writes
 */


static command_type_t read_command_from_flags(int flags)
{
	if((flags & EC_ADDR_AI) == EC_ADDR_AI)
		return cmd_ainc_r;
	if((flags & EC_ADDR_CA) == EC_ADDR_CA)
		return cmd_cadr_r;
	if((flags & EC_ADDR_BR) == EC_ADDR_BR)
		return cmd_bcst_r;
	if((flags & EC_ADDR_LG) == EC_ADDR_LG)
		return cmd_lgcl_r;
	return cmd_cadr_r;
}


static command_type_t write_command_from_flags(int flags)
{
	if((flags & EC_ADDR_AI) == EC_ADDR_AI)
		return cmd_ainc_w;
	if((flags & EC_ADDR_CA) == EC_ADDR_CA)
		return cmd_cadr_w;
	if((flags & EC_ADDR_BR) == EC_ADDR_BR)
		return cmd_bcst_w;
	if((flags & EC_ADDR_LG) == EC_ADDR_LG)
		return cmd_lgcl_w;
	return cmd_cadr_w;
}


static ethercat_operation_t* ec_create_operation(ethercat_t *ethercat)
{
	ethercat_operation_t *operation = (ethercat_operation_t *) malloc(sizeof(ethercat_operation_t));

	if(operation == NULL) {
		perror("malloc()");
		return NULL;
	}

	operation->command = cmd_noop;
	operation->length = 0;
	operation->flags = 0;

	operation->read_callback = NULL;
	operation->write_callback = NULL;
	operation->payload = NULL;

	operation->next = ethercat->operations;
	ethercat->operations = operation;

	return operation;
}


void ec_request_read(ethercat_t *ethercat, 
			const address_t address, 
			uint16_t length, 
			ec_read_callback_t *callback, 
			void *payload, 
			int flags)
{
	ethercat_operation_t *operation = ec_create_operation(ethercat);

	if(operation == NULL) {
		perror("ec_create_operation()");
		return;
	}

	operation->flags = flags;
	operation->command = read_command_from_flags(flags);
	operation->length = length;
	operation->read_callback = callback;
	operation->payload = payload;
}


void ec_request_write(ethercat_t *ethercat, 
			const address_t address, 
			uint16_t length, 
			ec_write_callback_t *callback, 
			void *payload, 
			int flags)
{
	ethercat_operation_t *operation = ec_create_operation(ethercat);

	if(operation == NULL) {
		perror("ec_create_operation()");
		return;
	}

	operation->flags = flags;
	operation->command = write_command_from_flags(flags);
	operation->length = length;
	operation->write_callback = callback;
	operation->payload = payload;
}


static int ec_get_packet_length(ethercat_t *ethercat)
{
	int length = 14 + 2 + 4;	// Ethernet + Ethercat + FCS

	ethercat_operation_t *operation = ethercat->operations;
	while(operation) {
		// Datagram header + Payload
		length += 10 + operation->length;
		operation = operation->next;
	}

	return length;
}


void ec_do_cycle(ethercat_t *ethercat)
{
	const uint8_t ethernet_hdr[] = {0x00, 0xd0, 0xb7, 0xbd, 0x22, 0x56, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x88, 0xa4};

	int length = ec_get_packet_length(ethercat);
	printf("Packet length: %02x\n", length);

	uint8_t *packet = (uint8_t *) malloc(length);

	if(packet == NULL) {
		perror("malloc()");
		return;
	}

	// Build packet
	uint8_t *ptr = packet;
	memcpy(ptr, ethernet_hdr, 14); ptr += 14;

	ptr[0] = length & 0x00FF;
	ptr[1] = (length & 0xFF00) >> 8;

/*    uint16_t tmp = buffer[0] | (buffer[1] << 8);
    header->length = tmp & 0x07FF;
    header->type = (payload_type_t) ((tmp & 0xF000) >> 12);
    buffer += 2;
  }*/
	
	printf("Sending:\n");
	for(int i = 0; i < length; i++) {
		printf("%02x ", packet[i]);
	}
	printf("\n");

	// Send packet and await response
	send(ethercat->socket, packet, length, MSG_DONTROUTE | MSG_DONTWAIT);
	int nbytes = read(ethercat->socket, (void *) packet, length);

	// Show packet
	decode(packet);

	free(packet);


	// Setup test frame
/*	uint8_t data[] = {0x00, 0xd0, 0xb7, 0xbd, 0x22, 0x56,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0x88, 0xa4,
                    0x0e, 0x10,

			// START OF DATAGRAM
										0x04, 	// Auto increment read (was 7 broadcast read)
										0x87, 
										0x02, 0x00, // Auto increment address 1
										0x30, 0x01, // AL status
										//0x10, 0x00,	// Address
										0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};	*/
}

/********************
 * Utility functions
 */

void print_header(ethercat_header_t *header)
{
	printf("From %02x", header->src_addr[0]);
	for(int i = 1; i < 6; i++)
		printf(":%02x", header->src_addr[i]);
	printf("; to: %02x", header->dst_addr[0]);
	for(int j = 1; j < 6; j++)
		printf(":%02x", header->dst_addr[j]);
	printf("; protocol: %04x; length: %02x; type: %01x\n", 
    header->proto_type, header->length, header->type);
}


void print_datagram(datagram_t *datagram)
{
  printf("Command: %s; Index: %02x; Address: %04x:%04x (%08x); Length: %d; Circ: %d; More: %d; Int: %04x; Wkc: %04x\n",
    command_description[(uint8_t) datagram->command],
    datagram->index,
    datagram->address.physical.ado,
    datagram->address.physical.adp,
    datagram->address.logical,
    datagram->length,
    datagram->circulating_frame,
    datagram->more_following,
    datagram->interrupt,
    datagram->working_counter);

  for(int i = 0; i < datagram->length; i++) {
    printf(" %02x", datagram->payload[i]);
  }
  printf("\n");
}


/**
 * Reads ethernet and ethercat header and return pointer to
 * first datagram.
 */
uint8_t *ec_read_header(uint8_t *buffer, ethercat_header_t *header)
{
  for(int i = 0; i < 6; i++)
    header->dst_addr[i] = buffer[i];
  buffer += 6;

  for(int i = 0; i < 6; i++)
    header->src_addr[i] = buffer[i];
  buffer += 6;

  header->proto_type = buffer[1] | (buffer[0] << 8);
  buffer += 2;

  if(header->proto_type == ETHERCAT_TYPE) {
    uint16_t tmp = buffer[0] | (buffer[1] << 8);
    header->length = tmp & 0x07FF;
    header->type = (payload_type_t) ((tmp & 0xF000) >> 12);
    buffer += 2;
  }

  return buffer;
}


/**
 * Reads datagram and returns point to next datagram
 */
uint8_t *ec_read_datagram(uint8_t *buffer, datagram_t *datagram)
{
  datagram->command = (command_type_t) buffer[0];
  datagram->index = buffer[1];
  datagram->address.physical.ado = buffer[2] | (buffer[3] << 8);
  datagram->address.physical.adp = buffer[4] | (buffer[5] << 8);

  uint16_t tmp = buffer[6] | (buffer[7] << 8);
  datagram->length = tmp & 0x07FF;
  datagram->circulating_frame = (tmp & 0x2000) >> 13;
  datagram->more_following = (tmp & 0x8000) >> 15;

  datagram->interrupt = buffer[8] | (buffer[9] << 8);

  buffer += 10;

  datagram->payload = buffer;
  buffer += datagram->length;

  datagram->working_counter = buffer[0] | (buffer[1] << 8);
  buffer += 2;

  return buffer;
}


void decode(uint8_t *buffer)
{
  ethercat_header_t header;
  datagram_t datagram;

  buffer = ec_read_header(buffer, &header);

  if(header.proto_type != ETHERCAT_TYPE) {
    fprintf(stderr, "Not an EtherCAT frame (found %04x instead of %04x).\n",
			header.proto_type, ETHERCAT_TYPE);
    return;
  }

  buffer = ec_read_datagram(buffer, &datagram);

  print_header(&header);
  print_datagram(&datagram);
}



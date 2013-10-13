
#include "ethercat_internal.h"

#include <stdio.h>


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

  header->proto_type = buffer[0] | (buffer[1] << 8);
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
    fprintf(stderr, "Not an EtherCAT frame.\n");
    return;
  }

  buffer = ec_read_datagram(buffer, &datagram);

  print_header(&header);
  print_datagram(&datagram);
}



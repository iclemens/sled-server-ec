
#include <string.h>
#include <stdio.h>

#include "ethercat.h"


void read_callback(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;

	printf("Received: %04x ", tmp[0] + (tmp[1] << 8));
	printf("(adress: %04x/%04x)\n", address.physical.ado, address.physical.adp);

}

int main()
{
	struct ethercat_t *ethercat = ec_create("eth2");

	address_t address;
	address.physical.ado = 0x0000;
	address.physical.adp = 0x0130;

	ec_request_read(ethercat, address, 2, read_callback, NULL, EC_CALL_ONESHOT | EC_ADDR_AI);

	ec_do_cycle(ethercat);

	// Writing to 0x0010 seems to set the address

	// Setup test frame
/*	uint8_t data[] = {
                    0x0e, 0x10,

			// START OF DATAGRAM
										0x04, 	// Auto increment read (was 7 broadcast read)
										0x87, 
										0x02, 0x00, // Auto increment address 1
										0x30, 0x01, // AL status
										//0x10, 0x00,	// Address
										0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	uint16_t size = 60;
	send(sock, (void *) data, size, MSG_DONTROUTE | MSG_DONTWAIT);

	// Wait for message
	uint8_t buffer[1024];
	int nbytes = read(sock, (void *) buffer, 1024);

  decode(buffer);

	close(sock);*/

	ec_destroy(&ethercat);

	return 0;
}


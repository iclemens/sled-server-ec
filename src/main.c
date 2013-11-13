
#include <string.h>
#include <stdio.h>

#include "ethercat.h"


void read_callback(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;

	printf("Received: %04x ", tmp[0] + (tmp[1] << 8));
	printf("(adress: %04x/%04x) ", address.physical.ado, address.physical.adp);
	printf("(wkc: %04x)\n", tmp[2] + (tmp[3] << 8));

}

void write_uint16(const address_t address, void *payload, uint16_t length, void *data)
{
	uint8_t *tmp = (uint8_t *) data;
	uint16_t *ptr = (uint16_t *) payload;

	tmp[0] = (*ptr) & 0xFF;
	tmp[1] = (*ptr) >> 8;
}

/**
 * Sets address of first device on bus.
 */
void configure_address(ethercat_t *ethercat, uint16_t address)
{
	address_t addr;
	addr.physical.ado = 0x0000;
	addr.physical.adp = 0x0010;

	ec_request_write(ethercat, addr, 2, write_uint16, (void *) &address, EC_CALL_ONESHOT | EC_ADDR_AI);
	ec_do_cycle(ethercat);
}


int main()
{
	struct ethercat_t *ethercat = ec_create("eth2");
	configure_address(ethercat, 1);

	address_t address;
	address.physical.ado = 0x0001;
	address.physical.adp = 0x0130;
	ec_request_read(ethercat, address, 2, read_callback, NULL, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);

	ec_destroy(&ethercat);

	return 0;
}


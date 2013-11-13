
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


struct mbox_t {
	uint16_t length;	// 0 1
	uint16_t address;	// 2 3
	uint8_t chan_prio;	// 4    0..5 = chan 6..7 = prio
	uint8_t type;		// 5    0..3
	uint8_t pdo;		// 6
	uint8_t coe_type;	// 7
	uint8_t control;	//8
	uint16_t index;
	uint8_t subindex;
	uint32_t data;
} __attribute__((packed));


void read_mboxin(const address_t address, void *payload, uint16_t length, const void *data)
{
	const mbox_t *mbox = (const mbox_t *) data;

	if(mbox->length == 0) return;

	printf("Mailbox response: ");
	
	printf("%02x: %04x:%02x = %08x", mbox->control, mbox->index, mbox->subindex, mbox->data);
	printf("\n");
}



void read_uint16(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint16_t *tmp = (const uint16_t *) data;
	uint16_t *out = (uint16_t *) payload;
	*out = *tmp;
}


void read_uint8(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;
	uint8_t *out = (uint8_t *) payload;
	*out = *tmp;
}



void read_status(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;
	uint16_t status = tmp[0] + (tmp[1] << 8);

	printf("Status: %04x ", status);

	switch(status & 0x0F) {
		case 0x01: printf("Init"); break;
		case 0x02: printf("PreOperational"); break;
		case 0x03: printf("Bootstrap mode"); break;
		case 0x04: printf("Safe operational"); break;
		case 0x08: printf("Operational"); break;
		default: printf("Unknown"); break;
	}
	printf("  ");

	if((status & 0x10) == 0x10)
		printf("(fault acknowledgement on)");
	else
		printf("(fault acknowledgement off)");

	printf("\n");
}



void read_status_code(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;
	uint16_t status = tmp[0] + (tmp[1] << 8);

	printf("Status: %04x ", status);

	switch(status) {
		case 0x0000: printf("No error"); break;
		case 0x0011: printf("Invalid state requested"); break;
		case 0x0017: printf("Invalid sync manager configuration"); break;
		case 0x001A: printf("Synchronize error"); break;
		default: printf("Invalid code"); break;
	}
	printf("\n");
}


void read_interrupt_enable(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;
	uint16_t status = tmp[0] + (tmp[1] << 8);
}


void write_payload(const address_t address, void *payload, uint16_t length, void *data)
{
	memcpy(data, payload, length);
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


void set_state(ethercat_t *ethercat, uint16_t state)
{
	address_t addr;
	addr.physical.ado = 0x0001;
	addr.physical.adp = 0x120;
	ec_request_write(ethercat, addr, 2, write_uint16, &state, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);
}

uint16_t get_state(ethercat_t *ethercat)
{
	uint16_t status;
	address_t addr;
	addr.physical.ado = 0x0001;
	addr.physical.adp = 0x130;
	ec_request_read(ethercat, addr, 2, read_uint16, &status, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);
	return status;
}
//S3152016AD5429001000

//S3152016AD64

/*0018 0002 2600 0101
  001C 0002 2200 0102
  0011 0000 2400 0103
  4011 0000 2000 0104*/

void write_sync_config(const address_t address, void *payload, uint16_t length, void *data)
{
	const uint8_t config[] = {
		// SM0 MailOut (master to drive)
		0x00, 0x18,  0x00, 0x02,  0x26,  0x00,  0x01, 0x00,
		// SM1 MailIn (drive to master)
		0x00, 0x1C,  0x00, 0x02,  0x22,  0x00,  0x01, 0x00,
		// SM2 ProOut
		0x00, 0x11,  0x06, 0x00,  0x24,  0x00,  0x01, 0x00,
		// SM3 ProIn
		0x40, 0x11,  0x06, 0x00,  0x20,  0x00,  0x01, 0x00
		// ADDRESS   LENGTH       CTL    STA (30?)    UNKNOWN
	};

	const uint8_t *cfg = config + address.physical.adp - 0x0800;
	memcpy(data, cfg, length);
}


void sync_read_callback(const address_t address, void *payload, uint16_t length, const void *data)
{
	const uint8_t *tmp = (const uint8_t *) data;
	for(int i = 0; i < length; i++) {
		printf("%02x ", tmp[i]);
		if((i+1) % 8 == 0)
			printf("\n");
	}
}


int main()
{
	struct ethercat_t *ethercat = ec_create("eth2");
	configure_address(ethercat, 1);

	address_t address;
	address.physical.ado = 0x0001;

	// Show fault and then reset
	if(get_state(ethercat) & 0x10) {
		address.physical.adp = 0x0134;
		ec_request_read(ethercat, address, 2, read_status_code, NULL, EC_CALL_ONESHOT);
		ec_do_cycle(ethercat);
		printf("Resetting faults.\n");
		set_state(ethercat, 0x01);
		set_state(ethercat, 0x11);
	}

	// Move to INIT
	set_state(ethercat, 0x01);
	while(get_state(ethercat) != 0x01) { }
	set_state(ethercat, 0x02);
	while(get_state(ethercat) != 0x02) { }
	printf("State is PreOperational\n\n");

	// Write sync manager config
	for(int i = 0; i < 4; i++) {
		address.physical.adp = 0x0800 | (i << 3);
		ec_request_write(ethercat, address, 8, write_sync_config, NULL, EC_CALL_ONESHOT);
		ec_do_cycle(ethercat);
	}

	// Read sync manager configruation
	address.physical.adp = 0x0800;
	ec_request_read(ethercat, address, 8 * 4, sync_read_callback, NULL, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);
	printf("\n");

	// SyncManager2 PDO
	const uint8_t mboxmsg1[] = {
		0x0A, 0x00, // Length
		0x00, 0x00, // Address 0
		0x00, // Channel 0
		0x03, // COE
		0x00, // PDO 0
		0x20, // PDO 0 and SDO-Request
		0x2B, // Write 2 bytes
		0x12, 0x1c, // Index
		0x01, // Subindex
		0x01, 0x17, 0x00, 0x00 // Data
	};

	// SyncManager3 PDO
	const uint8_t mboxmsg2[] = {
		0x0A, 0x00, // Length
		0x00, 0x00, // Address 0
		0x00, // Channel 0
		0x03, // COE
		0x00, // PDO 0
		0x20, // PDO 0 and SDO-Request
		0x2B, // Write 2 bytes
		0x13, 0x1c, // Index
		0x01, // Subindex
		0x01, 0x1B, 0x00, 0x00 // Data
	};

	uint8_t mboxout[512];
	uint8_t status;

	// SEND FIRST MESSAGE
	memset(mboxout, 0, 512);
	memcpy(mboxout, mboxmsg1, 16);
	address.physical.adp = 0x1800;
	ec_request_write(ethercat, address, 512, write_payload, mboxout, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);

	address.physical.adp = 0x080D;
	status = 0x00;

	while(!(status & 0x08)) {
		ec_request_read(ethercat, address, 1, read_uint8, &status, EC_CALL_ONESHOT);
		ec_do_cycle(ethercat);
	}

	// Read mailbox
	address.physical.adp = 0x1C00;
	ec_request_read(ethercat, address, 512, read_mboxin, NULL, EC_CALL_PERIODIC);
	ec_do_cycle(ethercat);

	// Clear mailbox
	memset(mboxout, 0, 512);
	address.physical.adp = 0x1800;
	ec_request_write(ethercat, address, 512, write_payload, mboxout, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);

	address.physical.adp = 0x080D;
	status = 0x08;

	while((status & 0x08)) {
		ec_request_read(ethercat, address, 1, read_uint8, &status, EC_CALL_ONESHOT);
		ec_do_cycle(ethercat);
	}




	// SEND SECOND MESSAGE
	memcpy(mboxout, mboxmsg2, 16);
	address.physical.adp = 0x1800;
	ec_request_write(ethercat, address, 512, write_payload, mboxout, EC_CALL_ONESHOT);
	ec_do_cycle(ethercat);

	address.physical.adp = 0x080D;
	status = 0x00;

	while(!(status & 0x08)) {
		ec_request_read(ethercat, address, 1, read_uint8, &status, EC_CALL_ONESHOT);
		ec_do_cycle(ethercat);
	}

	// Read mailbox
	address.physical.adp = 0x1C00;
	ec_request_read(ethercat, address, 16, read_mboxin, NULL, EC_CALL_PERIODIC);
	ec_do_cycle(ethercat);




	// Read AL event
	address.physical.adp = 0x134;
	ec_request_read(ethercat, address, 2, read_status_code, NULL, EC_CALL_PERIODIC);
	address.physical.adp = 0x130;
	ec_request_read(ethercat, address, 2, read_status, NULL, EC_CALL_PERIODIC);
	ec_do_cycle(ethercat);

	set_state(ethercat, 0x04);
	uint16_t state = 0x00;
	while( !(state & 0x14) ) {
		state = get_state(ethercat);
	}


	// Set cycle time
	//  Use mailbox to set 60C2 subindex 1 and 2 (PTBASE)

	// Start synchronization
	// .. Do I read and write the process words?!

	set_state(ethercat, 0x08);
	state = 0x00;
	/*while(state != 0x08) {
		state = get_state(ethercat);
	}*/

	ec_destroy(&ethercat);

	return 0;
}


#ifndef __EC_TYPES_H__
#define __EC_TYPES_H__

#include <stdint.h>

// Periodic or one-shot operations
#define EC_CALL_ONESHOT  0x01
#define EC_CALL_PERIODIC 0x02

// Addressing modes
#define EC_ADDR_AI       0x04
#define EC_ADDR_CA	 0x08
#define EC_ADDR_BR	 0x10
#define EC_ADDR_LG	 0x20


struct ethercat_t;

union address_t {
	struct {
		uint16_t ado;
		uint16_t adp;
	} physical;
	uint32_t logical;
};

typedef void(ec_read_callback_t)(const address_t, void *, uint16_t length, const void *);
typedef void(ec_write_callback_t)(const address_t, void *, uint16_t length, void *);

ethercat_t *ec_create(const char *);
void ec_destroy(ethercat_t **);

void ec_request_read(ethercat_t *, const address_t, uint16_t, ec_read_callback_t *, void *, int);
void ec_request_write(ethercat_t *, const address_t, uint16_t, ec_write_callback_t *, void *, int);

void ec_do_cycle(ethercat_t *ethercat);

#endif


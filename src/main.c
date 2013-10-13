
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netpacket/packet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "ethercat.h"

int open_socket(const char *interface)
{
	int sock;

	int ifindex;

	struct timeval timeout;
	struct ifreq ifr;
	struct sockaddr_ll sll;

	int flag_dont_route = 1;

	// Create raw EtherCAT socket
	sock = socket(PF_PACKET, SOCK_RAW, htons(ETHERCAT_TYPE));

	if(sock < 0) {
		perror("Could not create EtherCAT socket");
		return -1;
	}

	// Set timeout to 1 microsecond
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;

	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		perror("Could not set receive timeout.");
		close(sock);
		return -1;
	}

	if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
		perror("Could not set transmit timeout.");
		close(sock);
		return -1;
	}

	// Disable routing
	if(setsockopt(sock, SOL_SOCKET, SO_DONTROUTE, &flag_dont_route, sizeof(flag_dont_route)) == -1) {
		perror("Could not disable routing.");
		close(sock);
		return -1;
	}

	// Get interface index
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);
	if(ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
		perror("Could not get interface index.");
		close(sock);
		return -1;
	}
	ifindex = ifr.ifr_ifindex;

	// Get flags
	ifr.ifr_flags = 0;
	if(ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
		perror("Could not get socket flags.");
		close(sock);
		return -1;
	}

	// Update flags
	ifr.ifr_flags = ifr.ifr_flags | IFF_PROMISC | IFF_BROADCAST;
	if(ioctl(sock, SIOCSIFFLAGS, &ifr) == -1) {
		perror("Could not set socket flags.");
		close(sock);
		return -1;
	}

	// Bind socket
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifindex;
	sll.sll_protocol = htons(ETHERCAT_TYPE);

	if(bind(sock, (struct sockaddr *) &sll, sizeof(sll)) < 0) {
		perror("Could not bind socket");
		close(sock);
		return -1;
	}

	return sock;
}

void print_ethernet_header(ethernet_header_t *header)
{
	int i;

	printf("From: %02x", header->src_addr[0]);
	for(i = 1; i < 6; i++)
		printf(":%02x", header->src_addr[i]);

	printf("; To: %02x", header->dst_addr[0]);
	for(i = 1; i < 6; i++)
		printf(":%02x", header->dst_addr[i]);

	printf("; Type: %04x\n", header->type);
}

void decode_packet(uint8_t *buffer, uint16_t size)
{
	uint8_t *buffer_start = buffer;
	int i;

	// Read Ethernet header
	ethernet_header_t *ethernet_header = (ethernet_header_t *) buffer;
	ethernet_header->type = ntohs(ethernet_header->type);
	print_ethernet_header(ethernet_header);

	if(ethernet_header->type != ETHERCAT_TYPE) {
		fprintf(stderr, "Not an EtherCAT frame.\n");
		return;
	}
	
	buffer += sizeof(ethernet_header_t);

	// Read EtherCAT header
	uint16_t word1 = buffer[0] + (buffer[1] << 8);
	uint16_t length = word1 & 0x07FF;
	uint8_t type = (word1 & 0xF000) >> 12;
	uint8_t bit = (word1 & 0x800) >> 11;

	printf("Length: %d; type: %d; type: %02x\n", length, bit, type);
	buffer += 2;

	printf("\nDatagram:\n");

	// Read datagram
	uint8_t command = buffer[0];
	uint8_t index = buffer[1];
	uint32_t address = buffer[2] + (buffer[3] << 8) + (buffer[4] << 16) + (buffer[5] << 24);
	uint16_t word2 = buffer[6] + (buffer[7] << 8);
	uint8_t length1 = word2 & 0x07FF;
	uint8_t circulating_bit = (word2 & 0x2000) >> 13;
	uint8_t more_bit = (word2 & 0x8000) >> 15;
	//uint16_t interrupt = buffer[8] + (buffer[9] << 8);

	buffer += 1 + 1 + 4 + 2;

	printf(" Command: %02x; Index: %02x; Address: %08x\n", command, index, address);
	printf(" Length: %d; Circulating: %d; More: %d\n", length1, circulating_bit, more_bit);

	printf(" Payload:");
	for(i = 0; i < length1; i++) {
		printf(" %02x", buffer[0]);
		buffer += 1;
	}
	printf("\n");

	uint16_t wkc = buffer[0] + buffer[1] << 8;
	buffer += 2;

	uint32_t fcs = buffer[0] + buffer[1] << 8 + buffer[2] << 16 + buffer[3] << 24;
	buffer += 4;

	printf(" WKC: %d; FCS: %08x\n", wkc, fcs);

	printf("\n");
	uint64_t buf = (uint64_t) buffer;
	uint64_t buf2 = (uint64_t) buffer_start;
	printf("Processed: %d bytes\n", buf-buf2);
}

int main()
{
	int sock = open_socket("eth2");
	int i;

	uint8_t buffer[1024];
	uint16_t size = 0;

	// Setup broadcast message
	ethernet_header_t *frame = (ethernet_header_t *) buffer;
	frame->src_addr[0] = 0x00;
	frame->src_addr[1] = 0xd0;
	frame->src_addr[2] = 0xb7;
	frame->src_addr[3] = 0xbd;
	frame->src_addr[4] = 0x22;
	frame->src_addr[5] = 0x56;
	for(i = 0; i < 6; i++)
		frame->dst_addr[i] = 0xff;
	frame->type = htons(ETHERCAT_TYPE);
	size += sizeof(ethernet_header_t);

	// Setup ethercat header
	uint8_t data[] = {0x0e, 0x10, 0x07, 0x87, 0x01, 0x00, 0x30, 0x01, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	for(i = 0; i < 46;i ++)
		buffer[14 + i] = data[i];
	size = 60;

	send(sock, (void *) buffer, size, MSG_DONTROUTE | MSG_DONTWAIT);

	// Ethernet header + Ethercat header + DATA + working count

	// Wait for message
	printf("Waiting for data...\n");
	int nbytes = read(sock, (void *) buffer, 1024);

	decode_packet(buffer, nbytes);

	printf("Read %d bytes\n", nbytes);


	close(sock);

	return 0;
}


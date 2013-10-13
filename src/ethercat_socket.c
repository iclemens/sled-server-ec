#include "ethercat_internal.h"
#include "ethercat_socket.h"

#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netpacket/packet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

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


#ifndef DHCP_H
#define DHCP_H
#include <system.h>
#include <string.h>
#include <serial.h>
#include <arp.h>
#include <udp.h>
#include <rtl8139.h>

#define DHCP_REQUEST 1
#define DHCP_REPLY 2

#define DHCP_TRANSACTION_IDENTIFIER 0x55555555

typedef struct dhcp_packet {
    uint8_t op;
    uint8_t hardware_type;
    uint8_t hardware_addr_len;
    uint8_t hops;
    uint32_t xid;
    uint16_t seconds;
    uint16_t flags; // may be broken
    uint32_t client_ip; //
    uint32_t your_ip;
    uint32_t server_ip;
    uint32_t gateway_ip;
    uint8_t client_hardware_addr[16];
    uint8_t server_name[64];
    uint8_t file[128];
    uint8_t options[64];
} __attribute__ ((packed)) dhcp_packet_t;

int gethostaddr(char * addr);

void dhcp_discover();

void dhcp_request(uint8_t * request_ip);

void dhcp_handle_packet(dhcp_packet_t * packet);

void * get_dhcp_options(dhcp_packet_t * packet, uint8_t type);

void make_dhcp_packet(dhcp_packet_t * packet, uint8_t msg_type, uint8_t * request_ip);

#endif


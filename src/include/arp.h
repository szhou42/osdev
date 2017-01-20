#ifndef ARP_H
#define ARP_H
#include <system.h>
#include <ethernet.h>
#include <rtl8139.h>

#define ARP_REQUEST 1
#define ARP_REPLY 2

typedef struct arp_packet {
    uint16_t hardware_type;
    uint16_t protocol;
    uint8_t hardware_addr_len;
    uint8_t protocol_addr_len;
    uint16_t opcode;
    uint8_t src_hardware_addr[6];
    uint8_t src_protocol_addr[4];
    uint8_t dst_hardware_addr[6];
    uint8_t dst_protocol_addr[4];
} __attribute__((packed)) arp_packet_t;

typedef struct arp_table_entry {
    uint32_t ip_addr;
    uint64_t mac_addr;
} arp_table_entry_t;

void arp_handle_packet(arp_packet_t * arp_packet, int len);

void arp_send_packet(uint8_t * dst_hardware_addr, uint8_t * dst_protocol_addr);

int arp_lookup(uint8_t * ret_hardware_addr, uint8_t * ip_addr);

void arp_lookup_add(uint8_t * ret_hardware_addr, uint8_t * ip_addr);

void arp_init();

#endif

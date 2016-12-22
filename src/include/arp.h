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

void arp_handle_packet(arp_packet_t * arp_packet, int len);

void arp_reply();

#endif

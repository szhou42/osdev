#include <arp.h>
#include <rtl8139.h>


void arp_handle_packet(arp_packet_t * arp_packet, int len) {
    // Reply arp request
    if(arp_packet->opcode == ARP_REQUEST) {
        // Save some packet field
        char dst_hardware_addr[6];
        char dst_protocol_addr[4];
        memcpy(dst_hardware_addr, arp_packet->src_hardware_addr, 6);
        memcpy(dst_protocol_addr, arp_packet->src_protocol_addr, 4);

        // Set source MAC address, IP address (hardcode the IP address as 10.2.2.3 until we really get one..)
        get_mac_addr(arp_packet->src_hardware_addr);
        arp_packet->src_protocol_addr[0] = 10;
        arp_packet->src_protocol_addr[1] = 2;
        arp_packet->src_protocol_addr[2] = 2;
        arp_packet->src_protocol_addr[3] = 3;

        // Set destination MAC address, IP address
        memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
        memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);

        // Set opcode
        arp_packet->opcode = ARP_REPLY;

        // Set lengths
        arp_packet->hardware_addr_len = 6;
        arp_packet->protocol_addr_len = 4;

        // Set hardware type
        arp_packet->hardware_type = HARDWARE_TYPE_ETHERNET;

        // Now send it with ethernet
        ethernet_send_packet(dst_hardware_addr, arp_packet, sizeof(arp_packet_t), 0x0021);
    }
}

void arp_reply() {

}

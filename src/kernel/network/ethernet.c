#include <ethernet.h>
#include <serial.h>
#include <pci.h>
#include <network_utils.h>


int ethernet_send_packet(uint8_t * dst_mac_addr, uint8_t * data, int len, uint16_t protocol) {
    uint8_t src_mac_addr[6];
    ethernet_frame_t * frame = kmalloc(sizeof(ethernet_frame_t) + len);
    void * frame_data = (void*)frame + sizeof(ethernet_frame_t);

    // Get source mac address from network card driver
    get_mac_addr(src_mac_addr);

    // Fill in source and destination mac address
    memcpy(frame->src_mac_addr, src_mac_addr, 6);
    memcpy(frame->dst_mac_addr, dst_mac_addr, 6);

    // Fill in data
    memcpy(frame_data, data, len);

    // Fill in type
    frame->type = htons(protocol);

    // Send packet
    rtl8139_send_packet(frame, sizeof(ethernet_frame_t) + len);
    kfree(frame);

    //qemu_printf("Sent an ethernet packet, it looks like this\n");
    //xxd(frame, sizeof(ethernet_frame_t) + len);

    return len;
}

void ethernet_handle_packet(ethernet_frame_t * packet, int len) {
    void * data = (void*) packet + sizeof(ethernet_frame_t);
    int data_len = len - sizeof(ethernet_frame_t);
    // ARP packet
    if(ntohs(packet->type) == ETHERNET_TYPE_ARP) {
        qemu_printf("(ARP Packet)\n");
        arp_handle_packet(data, data_len);
    }
    // IP packets(could be TCP, UDP or others)
    if(ntohs(packet->type) == ETHERNET_TYPE_IP) {
        qemu_printf("(IP Packet)\n");
        ip_handle_packet(data, data_len);
    }
}

/*
 * Initialize the ethernet layer
 * */
void ethernet_init() {
    return;
}

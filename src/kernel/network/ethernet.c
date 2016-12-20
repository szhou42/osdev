#include <ethernet.h>
#include <pci.h>


int ethernet_send_packet(uint8_t * dst_mac_addr, uint8_t * data, int len, uint16_t protocol) {
    uint8_t src_mac_addr[6];
    ethernet_frame_t * frame = kmalloc(sizeof(ethernet_frame_t) + len);
    void * frame_data = (void*)frame + sizeof(ethernet_frame_t);
    // Get source mac address from e1000 driver
    get_mac_addr(src_mac_addr);

    // Fill in source and destination mac address
    memcpy(frame->src_mac_addr, src_mac_addr, 6);
    memcpy(frame->dst_mac_addr, dst_mac_addr, 6);

    // Fill in data
    memcpy(frame_data, data, len);

    // Fill in type, let's assume it's always an IP diagram right now
    frame->type = protocol;
    frame->type = 0x0021;

    // Send packet
    e1000_send_packet(frame, sizeof(ethernet_frame_t) + len);
    kfree(frame);
    return len;
}

/*
 * Initialize the ethernet layer
 * */
void ethernet_init() {
    return;
}

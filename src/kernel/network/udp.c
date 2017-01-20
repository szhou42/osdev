#include <udp.h>
#include <dhcp.h>
#include <ip.h>
#include <xxd.h>


uint16_t udp_calculate_checksum(udp_packet_t * packet) {
    // UDP checksum is optional in IPv4
    return 0;
}

void udp_send_packet(uint8_t * dst_ip, uint16_t src_port, uint16_t dst_port, void * data, int len) {
    int length = sizeof(udp_packet_t) + len;
    udp_packet_t * packet = kmalloc(length);
    memset(packet, 0, sizeof(udp_packet_t));
    packet->src_port = htons(src_port);
    packet->dst_port = htons(dst_port);
    packet->length = htons(length);
    packet->checksum = udp_calculate_checksum(packet);

    // Copy data over
    memcpy((void*)packet + sizeof(udp_packet_t), data, len);
    qemu_printf("UDP Packet sent\n");
    ip_send_packet(dst_ip, packet, length);
}

void udp_handle_packet(udp_packet_t * packet) {
    //uint16_t src_port = ntohs(packet->src_port);
    uint16_t dst_port = ntohs(packet->dst_port);
    uint16_t length = ntohs(packet->length);

    void * data_ptr = (void*)packet + sizeof(udp_packet_t);
    uint32_t data_len = length;
    qemu_printf("Received UDP packet, dst_port %d, data dump:\n", dst_port);
    xxd(data_ptr, data_len);

    if(ntohs(packet->dst_port) == 68) {
        dhcp_handle_packet(data_ptr);
    }
    return;
}

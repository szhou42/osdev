#include <ip.h>
#include <arp.h>
#include <printf.h>
#include <serial.h>
#include <ethernet.h>
#include <network_utils.h>
#include <dhcp.h>
#include <udp.h>

uint8_t my_ip[] = {10, 0, 2, 14};
uint8_t test_target_ip[] = {10, 0, 2, 15};
uint8_t zero_hardware_addr[] = {0,0,0,0,0,0};

void get_ip_str(char * ip_str, uint8_t * ip) {
    sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

uint16_t ip_calculate_checksum(ip_packet_t * packet) {
    // Treat the packet header as a 2-byte-integer array
    // Sum all integers up and flip all bits
    int array_size = sizeof(ip_packet_t) / 2;
    uint16_t * array = (uint16_t*)packet;
    uint8_t * array2 = (uint8_t*)packet;
    uint32_t sum = 0;
    for(int i = 0; i < array_size; i++) {
        sum += flip_short(array[i]);
    }
    uint32_t carry = sum >> 16;
    sum = sum & 0x0000ffff;
    sum = sum + carry;
    uint16_t ret = ~sum;
    return ret;
}

void ip_send_packet(uint8_t * dst_ip, void * data, int len) {
    int arp_sent = 3;
    ip_packet_t * packet = kmalloc(sizeof(ip_packet_t) + len);
    memset(packet, 0, sizeof(ip_packet_t));
    packet->version = IP_IPV4;
    // 5 * 4 = 20 byte
    packet->ihl = 5;
    // Don't care, set to 0
    packet->tos = 0;
    packet->length = sizeof(ip_packet_t) + len;
    // Used for ip fragmentation, don't care now
    packet->id = 0;
    // Tell router to not divide the packet, and this is packet is the last piece of the fragments.
    packet->flags = 0;
    packet->fragment_offset_high = 0;
    packet->fragment_offset_low = 0;

    packet->ttl = 64;
    // Normally there should be some other protocols embedded in ip protocol, we set it to udp for now, just for testing, the data could contain strings and anything
    // Once we test the ip packeting sending works, we'll replace it with a packet corresponding to some protocol
    packet->protocol = PROTOCOL_UDP;

    gethostaddr(my_ip);
    memcpy(packet->src_ip, my_ip, 4);
    memcpy(packet->dst_ip, dst_ip, 4);

    void * packet_data = (void*)packet + packet->ihl * 4;
    memcpy(packet_data, data, len);

    // Fix packet data order
    *((uint8_t*)(&packet->version_ihl_ptr)) = htonb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
    *((uint8_t*)(packet->flags_fragment_ptr)) = htonb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
    packet->length = htons(sizeof(ip_packet_t) + len);

    // Make sure checksum is 0 before checksum calculation
    packet->header_checksum = 0;
    packet->header_checksum = htons(ip_calculate_checksum(packet));


    //packet->header_checksum = htons(cksum(packet));
    // Don't care to pad, because we don't use the option field in ip packet
    /*
     * If the ip is in the same network, the destination mac address is the routers's mac address, the router'll figure out how to route the packet
     * Now, again, let's assume it's always in the same network, because i want to test if the simplest ip packet sending works as i write the code
     * */

    // Now look at the arp, table, if we have the mac address, just send it. If not, we'll send an arp packet to get the mac address, and wait until its mac address show up in
    // our arp  table

    uint8_t dst_hardware_addr[6];

    // Loop, until we get the mac address of the destination (this should probably done in a separate :))
    while(!arp_lookup(dst_hardware_addr, dst_ip)) {
        if(arp_sent != 0) {
            arp_sent--;
            // Send an arp packet here
            arp_send_packet(zero_hardware_addr, dst_ip);
        }
    }
    qemu_printf("IP Packet Sent...(checksum: %x)\n", packet->header_checksum);
    // Got the mac address! Now send an ethernet packet
    ethernet_send_packet(dst_hardware_addr, packet, htons(packet->length), ETHERNET_TYPE_IP);
    xxd(packet, ntohs(packet->length));
}


void ip_handle_packet(ip_packet_t * packet) {
    // Fix packet data order (be careful with the endiness problem within a byte)
    *((uint8_t*)(&packet->version_ihl_ptr)) = ntohb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
    *((uint8_t*)(packet->flags_fragment_ptr)) = ntohb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);

    qemu_printf("Receive: the whole ip packet \n");
    xxd(packet, ntohs(packet->length));
    // Now, the ip packet handler simply dumps ip header info and the data with xxd and display on screen
    // Dump source ip, data, checksum
    char src_ip[20];

    if(packet->version == IP_IPV4) {
        get_ip_str(src_ip, packet->src_ip);

        void * data_ptr = (void*)packet + packet->ihl * 4;
        int data_len = ntohs(packet->length) - sizeof(ip_packet_t);

        qemu_printf("src: %s, data dump: \n", src_ip);
        xxd(data_ptr, data_len);

        // If this is a UDP packet
        if(packet->protocol == PROTOCOL_UDP) {
            udp_handle_packet(data_ptr);
        }


        // What ? that's it ? that's ip packet handling ??
        // not really... u need to handle ip fragmentation... but let's make sure we can handle one ip packet first
    }
}

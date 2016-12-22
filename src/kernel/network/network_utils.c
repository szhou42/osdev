#include <network_utils.h>

uint16_t flip_short(uint16_t short_int) {
    uint32_t first_byte = *((uint8_t*)(&short_int));
    uint32_t second_byte = *((uint8_t*)(&short_int) + 1);
    return (first_byte << 8) | (second_byte);
}

uint32_t flip_long(uint32_t long_int) {
    uint32_t first_byte = *((uint8_t*)(&long_int));
    uint32_t second_byte = *((uint8_t*)(&long_int) + 1);
    uint32_t third_byte = *((uint8_t*)(&long_int)  + 2);
    uint32_t fourth_byte = *((uint8_t*)(&long_int) + 3);
    return (first_byte << 24) | (second_byte << 16) | (third_byte << 8) | (fourth_byte);
}

uint16_t htons(uint16_t hostshort) {
    return flip_short(hostshort);
}

uint32_t htonl(uint32_t hostlong) {
    return flip_long(hostlong);
}

uint16_t ntohs(uint16_t netshort) {
    return flip_short(netshort);
}

uint32_t ntohl(uint32_t netlong) {
    return flip_long(netlong);
}



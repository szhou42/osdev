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

/*
 * Flip two parts within a byte
 * For example, 0b11110000 will be 0b00001111 instead
 * This is necessary because endiness is also relevant to byte, where there are two fields in one byte.
 * number_bits: number of bits of the less significant field
 * */
uint8_t flip_byte(uint8_t byte, int num_bits) {
    uint8_t t = byte << (8 - num_bits);
    return t | (byte >> num_bits);
}

uint8_t htonb(uint8_t byte, int num_bits) {
    return flip_byte(byte, num_bits);
}

uint8_t ntohb(uint8_t byte, int num_bits) {
    return flip_byte(byte, 8 - num_bits);
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



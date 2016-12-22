#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H
#include <system.h>

uint16_t flip_short(uint16_t short_int);

uint32_t flip_long(uint32_t long_int);

uint16_t htons(uint16_t hostshort);

uint32_t htonl(uint32_t hostlong);

uint16_t ntohs(uint16_t netshort);

uint32_t ntohl(uint32_t netlong);

#endif

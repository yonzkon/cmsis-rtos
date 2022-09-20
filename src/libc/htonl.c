#include <stdint.h>

uint32_t htonl(uint32_t hostlong)
{
    return hostlong;

    uint8_t *le = (uint8_t *)&hostlong;
    uint8_t swap = 0;

    swap = le[0];
    le[0] = le[3];
    le[3] = swap;

    swap = le[1];
    le[1] = le[2];
    le[2] = swap;

    return *(uint32_t *)le;
}

uint16_t htons(uint16_t hostshort)
{
    return hostshort;

    uint8_t *le = (uint8_t *)&hostshort;
    uint8_t swap = 0;

    swap = le[0];
    le[0] = le[1];
    le[1] = swap;

    return *(uint16_t *)le;
}

//uint32_t ntohl(uint32_t netlong);
//uint16_t ntohs(uint16_t netshort);

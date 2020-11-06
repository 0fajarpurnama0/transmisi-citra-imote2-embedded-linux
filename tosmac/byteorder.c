#include "byteorder.h"


int is_host_msb(void)
{
   const uint8_t n[2] = {0,1};
   return ((*(uint16_t*)n) == 1);
}

int is_host_lsb(void)
{
   const uint8_t n[2] = {1,0};
   return ((*(uint16_t*)n) == 1);
}

uint16_t toLSB16(uint16_t a)
{
   return is_host_lsb() ? a:((a<<8)|(a>>8));
}

__u16 fromLSB16(__u16 a)
{
   return is_host_lsb() ? a:((a<<8)|(a>>8));
}


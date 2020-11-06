#ifndef __BYTE_ORDER
#define __BYTE_ORDER

#ifdef PSI
#include "CC2420_psi.h"
#else
#include <linux/module.h>
#include <linux/kernel.h>
#endif

int is_host_msb(void);
int is_host_lsb(void);
uint16_t toLSB16( uint16_t a );
__u16 fromLSB16( __u16 a );

#endif

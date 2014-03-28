

#define __STDC_CONSTANT_MACROS 
#include <inttypes.h>
#include <sys/types.h>

#ifndef CRC64_H
#define CRC64_H
extern uint64_t ls_crc64( uint64_t crc, const uint8_t * buf, size_t size);
//extern void ls_crc64_init(void);
#endif

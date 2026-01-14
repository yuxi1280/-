#ifndef __MIX_STD_LIB_H_
#define __MIX_STD_LIB_H_

#include <stdint.h>
#include <stddef.h>
//#include "stdarg.h"
// #include "types.h"

// #define uint8_t u8
// #define int8_t s8
// #define uint16_t u16
// #define int16_t s16
// #define uint32_t u32
// #define int32_t s32
// #define uint64_t u64
// #define int64_t s64

extern void __msl_memset(void *mem, uint8_t value, uint32_t len);
extern void __msl_memclr(void *mem, uint32_t len);
extern void __msl_memcpy(void *dst, void *src, uint32_t len);
extern int8_t __msl_memcmp(void *mem1, void *mem2, uint32_t len);
extern uint32_t __msl_strlen(char *str);
extern char * __msl_strstr(char *str1, char *str2);
extern void __msl_printf(const char *fmt_src, ...);

#endif


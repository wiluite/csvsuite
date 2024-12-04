/* stdint.h adapted for MS compilers */

#if defined(__cplusplus) 
#define __STDC_LIMIT_MACROS // we want the limit macros
#endif

#if !defined(_MSC_VER) || (_MSC_VER >= 1700)
/* Use compiler vendor stdint definitions */
   #include <stdint.h>
#else
/* No stdint.h in older Visual Studio versions */
   
#ifndef _STDINT_H
#define _STDINT_H


#ifdef _MSDOS
   typedef signed char    int8_t;
   typedef unsigned char  uint8_t;

   typedef short          int16_t;
   typedef unsigned short uint16_t;

   typedef long          int32_t;
   typedef unsigned long uint32_t;
#else
   typedef __int8           int8_t;
   typedef unsigned __int8  uint8_t;

   typedef __int16          int16_t;
   typedef unsigned __int16 uint16_t;

   typedef __int32          int32_t;
   typedef unsigned __int32 uint32_t;
   
   typedef __int64          int64_t;
   typedef unsigned __int64 uint64_t;
#endif

#if !defined ( __cplusplus) || defined (__STDC_LIMIT_MACROS)

#define INT16_MIN (-32768)

#define INT16_MAX 32767
#define UINT8_MAX 0xff /* 255U */
#define UINT16_MAX 0xffff /* 65535U */
#define UINT32_MAX 0xffffffff  /* 4294967295U */
#endif

#endif /* _STDINT_H */
#endif /* !_MSC_VER */

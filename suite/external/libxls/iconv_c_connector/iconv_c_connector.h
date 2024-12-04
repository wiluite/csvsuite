#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *iconv_t;

size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
iconv_t iconv_open(const char *tocode, const char *fromcode);
int iconv_close(iconv_t cd); 

#ifdef __cplusplus
}
#endif

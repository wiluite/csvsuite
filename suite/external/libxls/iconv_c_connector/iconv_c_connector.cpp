#include "iconv_c_connector.h"
#include <cppp/reiconv.hpp>

#ifdef __cplusplus
extern "C" {
#endif

size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft) {
    return cppp::base::reiconv::iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
}
iconv_t iconv_open(const char *tocode, const char *fromcode) {
    return cppp::base::reiconv::iconv_open(tocode, fromcode);
}
int iconv_close(iconv_t cd) {
    return cppp::base::reiconv::iconv_close(cd);
} 

#ifdef __cplusplus
}
#endif

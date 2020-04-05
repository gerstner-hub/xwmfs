#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 20200405L
#include <string.h>

/*
 * This is quite a mess: strerror_r has a GNU and an XSI variant. Non GNU-libc
 * like musl libc only provide the XSI variant. The signatures differ but the
 * names are equal. To export the XSI variant on GNU libc we need to #undef
 * _GNU_SOURCE. However, libstdc++ doesn't work without _GNU_SOURCE. Therefore
 * use this isolated compilation unit to wrap the XSI strerror_r to stay
 * portable.
 *
 * Even on musl libc _GNU_SOURCE seems to get defined under some circumstances
 * (c++ lib again?). A autoconf test could help here but messing with autoconf
 * seems more painful then doing it this way.
 */

namespace xwmfs
{

int xsi_strerror_r(int errnum, char *buf, size_t buflen)
{
	return strerror_r(errnum, buf, buflen);
}

} // end ns

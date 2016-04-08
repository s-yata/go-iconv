#ifndef ICONV_CGO_H
#define ICONV_CGO_H

#include <iconv.h>

/*
 * iconv_cgo_h is a handle type for character set conversion.
 *
 * iconv_cgo_h is not thread-safe and each thread should use each handle.
 */
typedef struct iconv_cgo_t *iconv_cgo_h;

/*
 * iconv_cgo_open() opens a handle that can convert a string from from_code to
 * to_code, such as from "CP932" to "UTF-8".
 *
 * to_code and from_code must be strings allocated via C.CString(). On success,
 * the ownership of to_code and from_code is passed to the opened handle and
 * the caller must not free to_code and from_code. On failure, the caller must
 * free to_code and from_code.
 *
 * //TRANSLIT and //IGNORE may be supported as special suffixes of to_code.
 *
 * On failure, iconv_cgo_open() sets errno and returns NULL.
 */
iconv_cgo_h iconv_cgo_open(char *to_code, char *from_code);

/*
 * iconv_cgo_close() closes a handle.
 *
 * On success, iconv_cgo_close() returns 0.
 * On failure, iconv_cgo_close() sets errno and returns -1.
 */
int iconv_cgo_close(iconv_cgo_h h);

/*
 * iconv_cgo_get_to_code() returns the destination character code.
 *
 * On failure, iconv_cgo_get_to_code() returns "".
 */
const char *iconv_cgo_get_to_code(iconv_cgo_h h);

/*
 * iconv_cgo_get_from_code() returns the source character code.
 *
 * On failure, iconv_cgo_get_from_code() returns "".
 */
const char *iconv_cgo_get_from_code(iconv_cgo_h h);

/*
 * iconv_cgo_conv() performs character set converion.
 *
 * iconv_cgo_conv() reads an input string indicated by in and in_size and
 * writes an output string into an internal buffer of h. Then, it assigns the
 * starting address and the size of the output string to *out and *out_size.
 *
 * The output string is available until the next call of iconv_cgo_conv() or
 * iconv_cgo_close() with the same handle.
 *
 * On success, iconv_cgo_conv() returns 0.
 * On failure, iconv_cgo_conv() sets errno and returns -1.
 */
int iconv_cgo_conv(iconv_cgo_h h, char *in, size_t in_size,
                   char **out, size_t *out_size);

#endif /* ICONV_CGO_H */
